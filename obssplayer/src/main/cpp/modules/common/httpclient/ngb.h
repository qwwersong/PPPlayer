#ifndef __NGB_H__
#define __NGB_H__

typedef struct ngb_rtmp_redirect
{
	int				flag;
	url_data_src	http;

	//http server
	char			uri[MAX_URL_SIZE];
	char			respath[MAX_URL_SIZE];

	char			ip[IP_LENGTH];
	int				validate;

	char			www[512];
	int				port;

}ngb_rtmp_redirect;

#ifdef __cplusplus
extern "C"
{
#endif

	int ngb_start_opt_rtmp_url(ngb_rtmp_redirect * h, const char *org_rtmp, int timeout );
	void ngb_reset_opt_rtmp(ngb_rtmp_redirect * h);
	void ngb_stop_opt_rtmp_url(ngb_rtmp_redirect * h);

#ifdef __cplusplus
}
#endif

#endif //__NGB_H__
