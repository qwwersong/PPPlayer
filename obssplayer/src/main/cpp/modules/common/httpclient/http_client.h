
#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#define IP_LENGTH			64
#define BUFFER_SIZE			8192
#define	MAX_URL_SIZE		2000
#define RING_SIZE			25

#define SPACE_CHARS " \t\r\n"

#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

#if defined(_MSC_VER)
#define strtoll			_strtoi64
#define strcasecmp		_stricmp
#define strncasecmp		_strnicmp
#endif

// data src reader context
typedef struct url_data_src
{
	//fd
	int		fd;

	//send and recv buffer
	int buf_size;
	unsigned char buffer[BUFFER_SIZE], *buf_ptr, *buf_end;

	//if >0 length =0,chunked(don't support)
	int filesize;
	int off;

	/**< Used if "Transfer-Encoding: chunked" otherwise -1. */
	int chunksize;

	char remote_ip[IP_LENGTH];

	//new location
	char location[MAX_URL_SIZE];

	//data recv
	int		consumed;
	int		data_alloc_size;
	unsigned char *	recv_memory;
	int		line_count;
	int		http_code;

	//
	int		work_flag;

	int		continue_pkt;
	int		slow_speed;

	int		total_bytes;

	/*	
		every data src must provide following interface 
		such as socket or local file
	*/
	// connect server, if it's a local file ,just return 1*/
	int (*url_read_connection)( struct url_data_src *s, char *addr, int port, int timeout); /* timeout ms */

	int (*url_http_request)(struct url_data_src *s, const char*host, const char *respath, const char *szItemName[], const char *szItemValue[],int itemSize);

	/* open the resource */
	int (*url_read_open)(struct url_data_src *s);

	/* read  data from src*/
	int (*url_read_data)(struct url_data_src *s);
	
	/* try to connect server, if it's local file, just return 1*/
	void (*url_read_reset)(struct url_data_src *s);

	/*close src*/
	void (*url_read_close)(struct url_data_src *s);

}url_data_src;


#ifdef __cplusplus
extern "C"
{
#endif
	//assign interface to data_src_reader
	void assign_http_reader(url_data_src *p);

#ifdef __cplusplus
}
#endif

#endif //__SMILE_H__