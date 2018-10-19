#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <iostream>

#include <curl/multi.h>

#define MAX_WAIT_MSECS 30*1000 /* Wait max. 30 seconds */

static const char *urls[] = {
  "https://api.seniverse.com/v3/weather/now.json?key=dydgtlvfx0mwzgrc&location=beijing&language=zh-Hans&unit=c",
  "https://api.seniverse.com/v3/weather/now.json?key=dydgtlvfx0mwzgrc&location=chongqing&language=zh-Hans&unit=c",
  "https://api.seniverse.com/v3/weather/now.json?key=dydgtlvfx0mwzgrc&location=shanghai&language=zh-Hans&unit=c",
  "https://api.seniverse.com/v3/weather/now.json?key=dydgtlvfx0mwzgrc&location=tianjin&language=zh-Hans&unit=c"
};
#define CNT 4

static size_t cb(char *d, size_t n, size_t l, void *p)
{
	/* take care of the data here, ignored in this example */
	(void)d;
	(void)p;

	std::cout << d << std::endl;

	return n * l;
}

static void init(CURLM *cm, int i)
{
	CURL *eh = curl_easy_init();
	curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, cb);
	curl_easy_setopt(eh, CURLOPT_HEADER, 0L);
	curl_easy_setopt(eh, CURLOPT_URL, urls[i]);
	curl_easy_setopt(eh, CURLOPT_PRIVATE, urls[i]);
	curl_easy_setopt(eh, CURLOPT_VERBOSE, 0L);
	curl_multi_add_handle(cm, eh);
}

int main(void)
{
	CURLM *cm = NULL;
	CURL *eh = NULL;
	CURLMsg *msg = NULL;
	CURLcode return_code = CURLE_OK;
	int still_running = 0, i = 0, msgs_left = 0;
	int http_status_code;
	const char *szUrl;

	curl_global_init(CURL_GLOBAL_ALL);

	cm = curl_multi_init();

	for (i = 0; i < CNT; ++i) {
		init(cm, i);
	}

	curl_multi_perform(cm, &still_running);

	do {
		int numfds = 0;
		int res = curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, &numfds);
		if (res != CURLM_OK) {
			fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
			return EXIT_FAILURE;
		}
		curl_multi_perform(cm, &still_running);

	} while (still_running);

	while ((msg = curl_multi_info_read(cm, &msgs_left))) {
		if (msg->msg == CURLMSG_DONE) {
			eh = msg->easy_handle;

			return_code = msg->data.result;
			if (return_code != CURLE_OK) {
				fprintf(stderr, "CURL error code: %d\n", msg->data.result);
				continue;
			}

			// Get HTTP status code
			http_status_code = 0;
			szUrl = NULL;

			curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);
			curl_easy_getinfo(eh, CURLINFO_PRIVATE, &szUrl);

			if (http_status_code == 200) {
				printf("200 OK for %s\n", szUrl);
			}
			else {
				fprintf(stderr, "GET of %s returned http status code %d\n", szUrl, http_status_code);
			}
			curl_multi_remove_handle(cm, eh);
			curl_easy_cleanup(eh);
		}
		else {
			fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
		}
	}

	curl_multi_cleanup(cm);

	return EXIT_SUCCESS;
}
//参考地址
//https://www.cnblogs.com/tinyfish/p/4719467.html
//https://www.cnblogs.com/liuhan333/p/5451828.html