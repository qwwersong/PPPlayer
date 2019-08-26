
#include "common.h"
#include "http_client.h"

#ifdef __ANDROID_BUILD__
//c-ares DNS
#include "cares/ares.h"

//-------------------------------add async dns resolve------------------------
static void callback(void *arg, int status, int timeouts, struct hostent *host)
{
    int i = 0;
    char ip[INET6_ADDRSTRLEN];
	url_data_src * out = (url_data_src *)arg;
	
    if(!host || status != ARES_SUCCESS){
        vpc_printf("dns error to lookup %s\n", ares_strerror(status));
        return;
    }

    for (i = 0; host->h_addr_list[i]; ++i) {
		inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
		if( out ){
			strcpy(out->remote_ip,ip);
			return;
		}
		/*
		if (host->h_aliases[0])  
		{  
			int i;  
			printf (", Aliases: ");  
			for (i = 0; host->h_aliases[i]; i++)  
				printf("%s ", host->h_aliases[i]);
		}  */
	}
	memset( out->remote_ip, 0, sizeof(out->remote_ip) );
}

static void wait_ares(url_data_src *http, ares_channel channel )
{
	int wait_time = 0;
    while( http->work_flag )
	{
		int need_process = 0;
  		fd_set read_fds, write_fds;
		
		if( wait_time == 0 )
		{
			struct timeval *tvp, tv;
			tvp = ares_timeout(channel, NULL, &tv);
			if( tvp ){
				wait_time = tvp->tv_sec*1000;
				wait_time +=tvp->tv_usec/1000; 
			}
			else{
				wait_time = 100;
			}
		}
		
		struct timeval tt;
		int nfds;

		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		nfds = ares_fds(channel, &read_fds, &write_fds);
		if(nfds == 0){
			break;
		}
		
		tt.tv_sec = 0;
		tt.tv_usec = 100*1000;
		if( !select(nfds, &read_fds, &write_fds, NULL, &tt) )
			wait_time -= 100;
		else
			wait_time = 0;
		
		if(wait_time <= 0 ) {
			need_process = 1;
			wait_time = 0;
		}
		
		if(need_process)
			ares_process(channel, &read_fds, &write_fds);
    }
}

char * gethostbyname_async( url_data_src *http, char *hostname )
{
	ares_channel channel;
	int status;

	memset(http->remote_ip,0,sizeof(http->remote_ip));

	status = ares_library_init(ARES_LIB_INIT_ALL);
	if (status != ARES_SUCCESS){
		vpc_printf("ares_library_init: %s\n", ares_strerror(status));
		return 0;
	}
	status = ares_init(&channel);
	if(status != ARES_SUCCESS) {
		vpc_printf("ares_init: %s\n", ares_strerror(status));
		return 0;
	}

	ares_gethostbyname(channel, hostname, AF_INET, callback, (void*)http);
	wait_ares(http, channel);
	
    ares_destroy(channel);
    ares_library_cleanup();
	
	if( !strlen( http->remote_ip ) )
		return 0;
	
	//return remote ip
	return http->remote_ip;
}

//--------------------------------------------------------------------
#endif 

#ifdef __IOS_BUILD__

#include <MacTypes.h>
#include <unistd.h>
#include <fcntl.h>
#import <CFNetwork/CFHost.h>
#import <resolv.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>

void DNSResolverHostClientCallback ( CFHostRef theHost, CFHostInfoType typeInfo, const CFStreamError *error, void *info) 
{
	int  *pCode = (int*)info;
	if ( error->domain || error->error ){
		printf("Dns error in ClientCallback\n");
		*pCode = -1;
	}
	else{
		*pCode = 1;
	}
}

char * gethostbyname_async( url_data_src *http,char *hostname )
{
	memset(http->remote_ip,0,sizeof(http->remote_ip));
	CFHostRef hostRef;
	CFArrayRef addresses = NULL;

	/* dns result code */
	int iDNSCode = 0; /* 0---doing, 1---ok, -1----error */

	CFStringRef hostNameRef = CFStringCreateWithCString(kCFAllocatorDefault, hostname, kCFStringEncodingASCII);

	hostRef = CFHostCreateWithName(kCFAllocatorDefault, hostNameRef);
	if( !hostRef )
		return 0;

	/* set async callback */
	CFHostClientContext ctx = {.info = (void*)&iDNSCode };
	CFHostSetClient(hostRef, DNSResolverHostClientCallback, &ctx);

	/* run engine */
	CFRunLoopRef runloop = CFRunLoopGetCurrent();
	CFHostScheduleWithRunLoop(hostRef, runloop, CFSTR("DNSResolverRunLoopMode"));

	if (hostRef) {
		Boolean didStart = CFHostStartInfoResolution(hostRef, kCFHostAddresses, NULL);
		if ( !didStart ) {
			printf("can't start CFHostStartInfoResolution\r\n");
			return 0;
		}
	}

	while( http->work_flag && ( iDNSCode == 0 ) ) {
		CFRunLoopRunInMode(CFSTR("DNSResolverRunLoopMode"), 0.02, true);
	}

	if( !http->work_flag ){
		/* cancel DNS */
		CFHostCancelInfoResolution( hostRef, kCFHostAddresses );
	}

	/* check result, 1 for OK */
	if( iDNSCode == 1 )
	{
		struct sockaddr_in* remoteAddr = 0;
		Boolean result;
		/* get result */
		addresses = CFHostGetAddressing(hostRef, &result);
		for(int i = 0; i < CFArrayGetCount(addresses) ; i++)
		{
			CFDataRef saData = (CFDataRef)CFArrayGetValueAtIndex(addresses, i);
			remoteAddr = (struct sockaddr_in*)CFDataGetBytePtr(saData);
			if(remoteAddr != NULL)
			{
				if( remoteAddr->sin_family == AF_INET ){

					const char *pstr = inet_ntop(AF_INET, &remoteAddr->sin_addr, http->remote_ip, IP_LENGTH);
					vpc_printf("ip4=%s,%s\r\n",pstr,http->remote_ip);
				}
				else if( remoteAddr->sin_family == AF_INET6 ){
					struct sockaddr_in6* remoteAddr2 = (struct sockaddr_in6*)remoteAddr;
					const char *pstr = inet_ntop(AF_INET6, &remoteAddr2->sin6_addr, http->remote_ip, IP_LENGTH);
					vpc_printf("ip6=%s,%s\r\n",pstr,http->remote_ip);
				}
				break;//only need 1
			}
		}
	}
	CFRelease(hostNameRef);
	CFRelease(hostRef);

	if( !strlen(http->remote_ip) )
		return 0;

	//return remote ip
	return http->remote_ip;
}

#endif

static int http_get_line(url_data_src *s, char *line, int line_size);
static int process_line(url_data_src *s, char *line,int *new_location);

static int set_non_blocking( int fd )
{
#ifdef WIN32
	unsigned long nonblocking = 1;
	ioctlsocket( fd, FIONBIO, (unsigned long*) &nonblocking );
#else
	int flags = fcntl( fd, F_GETFL, 0 );
	fcntl( fd, F_SETFL, flags | O_NONBLOCK );
#endif
	return 1;
}

static int add_addr_info(url_data_src *http,struct addrinfo **service, char *addr, int port)
{
	struct addrinfo hints;
	char portNo[32];
	int ret = 1;
	sprintf(portNo, "%d", port);
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype	= SOCK_STREAM;
	hints.ai_family		= AF_UNSPEC;

#ifdef _WIN32
	if( getaddrinfo(addr, portNo, &hints, service ) != 0 ){
		ret = 0;
	}
#else
	if( getaddrinfo(gethostbyname_async(http, addr), portNo, &hints, service ) != 0 ){
		ret = 0;
	}
#endif
	return ret;
}

static int make_connection( url_data_src *http, char *addr, int port, int timeout )
{
	const int try_times = 3;
	int n = 0,consume_time = 0, try_connect = try_times;
	int en, len = sizeof(en), connect_ok = 0;
	int socketFd;
//	int new_location = 0;
	int addrlen;
	
	/* DNS */
	struct addrinfo *service;
	if ( !add_addr_info(http, &service, addr, port ) ){
		return 0;
	}

	//create socket
	socketFd = socket( service->ai_family, service->ai_socktype, service->ai_protocol );
	if(socketFd < 0)
		return 0;

	set_non_blocking( socketFd );

	while( http->work_flag && try_connect && !connect_ok )
	{
		fd_set  WriteSet;
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 50*1000;

		if( !consume_time ){
			n = connect(socketFd, service->ai_addr, service->ai_addrlen );
		}

		FD_ZERO( &WriteSet );
		FD_SET(socketFd, &WriteSet);
		n = select(socketFd+1,0,&WriteSet,0,&tv);
		if(  n > 0 ) 
		{
			getsockopt(socketFd, SOL_SOCKET, SO_ERROR, &en, &len); 
			if( en == 0 ) 
			{
				//connect is OK;
				connect_ok = 1;
				break;
			}
		}
		else if( n == 0){
			consume_time += 50;
		}
		else if( n < 0) {
			consume_time = 0;
			try_connect--;
		}
		if( consume_time >= timeout/try_times) {
			consume_time = 0;
			try_connect--;
		}
	}
	http->fd = socketFd;
	return connect_ok;
}


static int send_http_request(url_data_src *http)
{
	int n = 0;
	char *sbuf	= http->buffer;
	int size	= http->buf_size;

	while( http->work_flag && size )
	{
		fd_set  WriteSet;
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 80*1000;

		FD_ZERO( &WriteSet );
		FD_SET( http->fd, &WriteSet );
		n = select(http->fd + 1,0,&WriteSet,0,&tv);
		if(  n > 0 ) 
		{
			n = send(http->fd,sbuf, size, 0);
			if( n <= 0 ){
				return -1;
			}
			sbuf += n;
			size -= n;
		}
		else if( n<0) {
			return  0;
		}
	}
	http->buf_size = 0;
	return 1;
}

//return header length
int http_request_header( url_data_src *s, const char *host, const char *respath,
	const char *szItemName[], 
	const char *szItemValue[],
	int itemSize)
{
	char *p = (char*)s->buffer;
	int num, i=0;

	s->buf_size = 0;
	s->http_code = -1;

	num = sprintf(p,"GET %s HTTP/1.1\r\n",respath);
	p += num;

	num = sprintf(p,"Host: %s\r\n", host);
	p +=num;

	num = sprintf(p,"User-Agent: bacHttp\r\n");
	p +=num;

	num = sprintf(p,"Accept: */*\r\n");
	p += num;

// 	num = sprintf(p,"Connection: Keep-Alive\r\n");
// 	p +=num;

	for ( i=0; i<itemSize; i++ ){
		num = sprintf(p,"%s: %s\r\n",szItemName[i], szItemValue[i]);
		p +=num;
	}
	num = sprintf(p,"\r\n");
	p += num;

	s->buf_size = ((uint8_t*)p)-s->buffer;

	return s->buf_size;
}

static int parse_http_response(url_data_src *s , int *new_location)
{
	int err;
	static char line[1024];
    /* init input buffer */
	s->line_count = 0;
    s->buf_ptr = s->buffer;
    s->buf_end = s->buffer;
    s->chunksize = -1;
	s->filesize = -1;

    /* wait for header */
	while( s->work_flag )
	{
        if (http_get_line(s, line, sizeof(line)) < 0)
            return -1;

        err = process_line(s, line,new_location);
        if (err < 0)
            return err;
        if (err == 0)
            break;
        s->line_count++;
    }
	return 1;
}

static int http_open(url_data_src *s )
{
	int new_location = 0;
	if( (send_http_request(s) == 1) && (parse_http_response(s,&new_location) == 1)){
		return s->http_code;
	}
	return -1;
}

static int http_recv_reponse_data(url_data_src *http, unsigned char *buf, int size)
{
	int ret = 0;
	int timeout = 0;
	while( http->work_flag )
	{
		fd_set  readSet;
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 80*1000;

		FD_ZERO( &readSet );
		FD_SET( http->fd, &readSet );
		ret = select(http->fd + 1,&readSet,0,0,&tv);
		if(  ret > 0 ) 
		{
			ret = recv(http->fd,buf,size,0);
			if( ret <= 0 ){
				ret = -1;
				//printf("recv return=%d\r\n",ret);
			}
			else
			{
				http->total_bytes += ret;
				http->continue_pkt++;
			}
			return ret;
		}
		else if( ret<0) {
			http->continue_pkt = 0;
			return  -1;
		}
		else if( ret == 0){
			http->continue_pkt = 0;
			//time out
			timeout++;
			if( timeout>45) //5 seconds
				return -1;//timeout
		}
	}
	return -1;
}

static int http_getc(url_data_src *s)
{
	int len;
	if (s->buf_ptr >= s->buf_end)
	{
		len = http_recv_reponse_data(s,s->buffer,BUFFER_SIZE);
		if (len < 0) {
			return -1;
		} else if (len == 0) {
			return -1;
		} else {
			s->buf_ptr = s->buffer;
			s->buf_end = s->buffer + len;
		}
	}
	return *s->buf_ptr++;
}

static int http_get_line(url_data_src *s, char *line, int line_size)
{
	int ch;
	char *q;

	q = line;
	for(;;) {
		ch = http_getc(s);
		if (ch < 0)
			return -1;
		if (ch == '\n') {
			/* process line */
			if (q > line && q[-1] == '\r')
				q--;
			*q = '\0';

			return 0;
		} else {
			if ((q - line) < line_size - 1)
				*q++ = ch;
		}
	}
	return 0;
}

static int process_line(url_data_src *s, char *line,int *new_location)
{
    char *tag, *p, *end;

    /* end of header */
    if (line[0] == '\0')
        return 0;

    p = line;
    if (s->line_count == 0) {
        while (!isspace(*p) && *p != '\0')
            p++;
        while (isspace(*p))
            p++;
        s->http_code = strtol(p, &end, 10);

        /* error codes are 4xx and 5xx, but regard 401 as a success, so we
         * don't abort until all headers have been parsed. */
//         if (s->http_code >= 400 && s->http_code < 600 && (s->http_code != 401
//             || s->auth_state.auth_type != HTTP_AUTH_NONE) &&
//             (s->http_code != 407 || s->proxy_auth_state.auth_type != HTTP_AUTH_NONE))
// 		if( s->http_code != 200 && s->http_code != 302 && s->http_code != 301  )
// 		{
//             end += strspn(end, SPACE_CHARS);
//             printf("HTTP error %d %s\n",s->http_code, end);
//             return -1;
//         }
    }
	else
	{
        while (*p != '\0' && *p != ':')
            p++;
        if (*p != ':')
            return 1;

        *p = '\0';
        tag = line;
        p++;
        while (isspace(*p))
            p++;
        if (!strcasecmp(tag, "Location")) {
            strcpy(s->location, p);
            *new_location = 1;
        } else if (!strcasecmp (tag, "Content-Length") && s->filesize == -1) {
            s->filesize = atoi(p);
        } else if (!strcasecmp (tag, "Content-Range")) {
            /* "bytes $from-$to/$document_size" */
            const char *slash;
			if (!strncmp (p, "bytes ", 6)) {
				p += 6;
				s->off = atoi(p);
				if ((slash = strchr(p, '/')) && strlen(slash) > 0)
					s->filesize = atoi(slash+1);
			}
		} else if (!strcasecmp (tag, "Transfer-Encoding") && !strncasecmp(p, "chunked", 7)) {
			s->filesize = -1;
			s->chunksize = 0;
        }
    }
    return 1;
}

static int http_buf_read(url_data_src *s, unsigned char *buf, int size)
{
	int len;
	/* read bytes from input buffer first */
	len = s->buf_end - s->buf_ptr;
	if (len > 0) {
		if (len > size)
			len = size;
		memcpy(buf, s->buf_ptr, len);
		s->buf_ptr += len;
	} else {
		len = http_recv_reponse_data(s,buf,size);
	}
	if (len > 0) {
		s->off += len;
		if (s->chunksize > 0)
			s->chunksize -= len;
	}
	return len;
}

//-1 err
//1 ok

static int http_read_data(url_data_src *s)
{
	int size = 0;

	s->consumed = 0;

	if( s->filesize > 0 )
	{
		if( s->data_alloc_size < s->filesize )
		{
			if( s->recv_memory )
				vpc_mem_free(s->recv_memory);

			s->data_alloc_size = (s->filesize + 15)&(~15);
			s->recv_memory = (unsigned char *)vpc_mem_alloc(s->data_alloc_size);
			if( !s->recv_memory ) 
				return -1;
		}
	}

	while ( s->work_flag )
	{
		if(s->slow_speed && s->continue_pkt > 5 )
		{
			s->continue_pkt = 0;
			vpc_delay(80);
			continue;;
		}
		if (s->chunksize >= 0) 
		{
			if (!s->chunksize) 
			{
				char line[32];
				for(;;) {
					do {
						if (http_get_line(s, line, sizeof(line)) < 0)
							return -1;
					} while (!*line);    /* skip CR LF from last chunk */

					s->chunksize = strtoll(line,0,16);
					if (!s->chunksize)
						return 1;
					break;
				}
			}
			size = s->data_alloc_size - s->consumed;
			if( size < s->chunksize )
			{
				unsigned char *t = 0;
				s->data_alloc_size = (s->consumed + s->chunksize*5 + 15)&(~15);
				t = (unsigned char *)vpc_mem_alloc(s->data_alloc_size);
				if( !t ) 
					return -1;
				if( s->recv_memory)
				{
					if(s->consumed)
						memcpy(t,s->recv_memory,s->consumed);
					vpc_mem_free(s->recv_memory);
				}
				s->recv_memory = t;
			}

			size = s->data_alloc_size - s->consumed;
			size = FFMIN(size, s->chunksize);
		}
		else
		{
			size = s->data_alloc_size - s->consumed;
		}

		size = http_buf_read(s,s->recv_memory+s->consumed,size);
		if( size == -1 )
			return -1;
		s->consumed += size;
		if( s->filesize > -1 && s->consumed >= s->filesize )
			return 1;
	}
	return 1;
}

static void http_reset(url_data_src *s)
{
    if( s->fd )
    {
#ifdef WIN32
        closesocket(s->fd);
#else
        close(s->fd);
#endif
		s->fd = 0;
    }
	s->http_code = -1;
	s->work_flag = 1;
}

static void http_close(url_data_src *s)
{
	if( s->recv_memory )
		vpc_mem_free(s->recv_memory);

	if( s->fd )
	{
#ifdef WIN32
		closesocket(s->fd);
#else
		close(s->fd);
#endif
	}
	memset(s,0,sizeof(*s));
}

void assign_http_reader(url_data_src *p)
{
	p->url_read_connection = make_connection;
	p->url_http_request = http_request_header;
	p->url_read_open = http_open;
	p->url_read_data = http_read_data;
	p->url_read_reset = http_reset;
	p->url_read_close = http_close;
}
