#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <json.h>

struct string {
  char *ptr;
  size_t len;
};

struct curl_slist *header = NULL;
char * token = "Authorization: Bot token";

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

void send_message (CURL *curl, char* message) {
	CURLcode res;
	struct string s;
	
	init_string(&s);
	
	curl_easy_setopt(curl, CURLOPT_URL, "https://discordapp.com/api/channels/222739089009541130/messages");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	/* Perform the request, res will get the return code */ 
	res = curl_easy_perform(curl);
	/* Check for errors */ 
	if(res != CURLE_OK)
	  fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
	json_object *result, *a;
	result = json_tokener_parse(s.ptr);
  int error;

  error = json_object_object_get_ex(result, "message", &a);
  if (error == 0) {
	  printf("%s", json_object_to_json_string_ext(result, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
  }
  else {
    printf("Error: \"%s\"!\n", json_object_get_string(a));
  }
}
int connect_to_gateway (CURL *curl) {
	struct string s;
	
	init_string(&s);
	
	CURLcode res;
	curl_easy_setopt(curl, CURLOPT_URL, "https://discordapp.com/api/gateway");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	/* Perform the request, res will get the return code */ 
	res = curl_easy_perform(curl);
	/* Check for errors */ 
	if(res != CURLE_OK)
	  fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		return -1;
	printf("%s \n", s.ptr);
	return 0;
}

int main (int argc, char* argv[]) {
	header = curl_slist_append(header, token);
	
	CURL *curl;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();

	if(curl) {
		send_message(curl, "{\"content\" : \"test\"}");
		//connect_to_gateway(curl);
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
}