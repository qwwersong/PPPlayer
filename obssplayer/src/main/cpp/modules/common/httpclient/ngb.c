
#include "common.h"

static int parse_rtmp_uri(const char *uri, int size,char *www, char *respath, int *port)
{
	char /*proto_str[128],*/*ptr;
	size_t proto_len = strspn(uri, URL_SCHEME_CHARS);
    if( proto_len  && strncmp(uri,"rtmp://",7) == 0 )
    {
        int len = 0;
        if( uri[proto_len] != ':')
            return 0;
        proto_len += 3;//skip ://
        uri += proto_len;
        
        ptr = strstr(uri,"/");
        if(ptr)
        {
            len = ptr-uri;
            memcpy(www,uri,len);
            www[len] = '\0';
            
            len = uri+size-ptr;
            memcpy(respath,ptr,len);
            respath[len] = '\0';
        }
        ptr = strstr(www,":");
        if( ptr )
        {
            *ptr = '\0';
            ptr++;
            *port = atoi(ptr);
        }
        if(!*port)
            *port = 1935;
    }
    else
    {
        
        memcpy(respath,uri,size);
        respath[size] = '\0';
    }
    return 1;
}

void map_opt_url(ngb_rtmp_redirect * p)
{
	sprintf(p->uri,"rtmp://%s%s&wsHost=%s&wsiphost=ipdb",p->ip, p->respath, p->www );
}

int splite_ip(ngb_rtmp_redirect * p)
{
	url_data_src *h = &p->http;
	char * s = h->recv_memory;
	int len = h->filesize;
	int ip_num = 0;
	int line_size = IP_LENGTH;
	char *line = p->ip;

	while( len > 0)
	{
		int ch;
		char *q;

		q = line;
		for(;;) {
			ch = *s++;
			len--;
			if (ch == '\n') {
				/* process line */
				if (q > line && q[-1] == '\r')
					q--;
				*q = '\0';
				ip_num++;
				break;
			} else {
				if ((q - line) < line_size - 1)
					*q++ = ch;
			}
		}
		if( ip_num )
			break;
	}
	return ip_num;
}

int ngb_start_opt_rtmp_url(ngb_rtmp_redirect * p, const char *org_rtmp, int timeout )
{
    url_data_src *h = &p->http;
    int reconnect_times = 0;
    int need_reconnect = 1;
    int ret = 0;

	parse_rtmp_uri(org_rtmp,(int)strlen(org_rtmp), p->www, p->respath, &p->port);
	if( p->validate ){
		map_opt_url(p);
		return 1;
	}

	/* get opt ip from server */
	assign_http_reader(h);
	p->flag = 1;
    
    while( p->flag )
    {
		const char *szName[8] = {0};
		const char *szValue[8] = {0};
		int itemsize = 0;

        //process request
        if( need_reconnect )
        {
            reconnect_times++;
			if( reconnect_times > 2 ){
				return ret;
			}
            h->url_read_reset(h);
             if( h->url_read_connection(h, "sdk.wscdns.com", 80, timeout )!= 1 ){
                need_reconnect = 1;
				vpc_printf("can't to http server,try again\r\n");
//                 if( reconnect_times > 5 ){
//                     p->notify_routine( p->key, MSG_BY_PASS, VPC_NET_TIME_OUT,0);
//                 }
                continue;
            }
            else{
                need_reconnect = 0;
                reconnect_times = 0;
            }
        }

		/* special request keyword*/
		szName[itemsize] = "WS_URL";
		szValue[itemsize++] = org_rtmp+7;

		szName[itemsize] = "WS_RETIP_NUM";
		szValue[itemsize++] = "3";

		szName[itemsize] = "WS_URL_TYPE";
		szValue[itemsize++] = "1";

        //get a http request header
		ret = h->url_http_request(h, "sdk.wscdns.com","/", szName, szValue, itemsize );
        ret = h->url_read_open(h);
		/*
		if( ret == 302)
		{
			memset(h->remote_ip,0,sizeof(h->remote_ip));
			parse_rtmp_uri(h->location,(int)strlen(h->location),p->www,p->respath,&p->port);
			need_reconnect = 1;
		}
		*/
		if( ret == 200 )
		{
			ret = h->url_read_data(h);

			if( splite_ip(p)){
				p->validate = 1;
				ret = 1;
			}
		}
		else{
			ret = 0;
		}
		p->flag = 0;
    }
    h->url_read_close(h);

	if( p->validate ){
		map_opt_url(p);
	}

	return p->validate;
}

void ngb_stop_opt_rtmp_url(ngb_rtmp_redirect * p)
{
	p->flag = 0;
	p->http.work_flag = 0;
}

void ngb_reset_opt_rtmp(ngb_rtmp_redirect * p)
{
	p->validate = 0;
}

