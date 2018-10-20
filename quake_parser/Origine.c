/*
 * Scarica il file in ordine di magnitudo
 * Prendi per ogni linea Time, Magnitude, Event Location name
 * foreach riga in terremoti
 * i = magnitudo; while(1)
 * for i..12
 */
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include "useful.h"

//#include <curl\curl.h>

int **contacts;
int *indexes;

int contacts_parser(void) {
	FILE *subscriber = fopen("./quaker.json", "r");
	if (!subscriber) {
		fprintf(stderr, "File not opened, errno %d - %s\n", errno, strerror(errno));
		return EXIT_FAILURE;
	}
	char *s;
	while (reader(subscriber, &s, 4));	// Discard {
	free(s);
	while (reader(subscriber, &s, 16));	// Discard "levels"
	free(s);
	while (reader(subscriber, &s, 16));
	int position, chat_id = 0, index = 0, level = 0, code;
	while (level < 13) {
		position = 0, index = 0;
		while (s[position] != '[') position++;
		if (s[++position] == ']') {
			indexes[level] = index;
			level++;
			free(s);
			while (reader(subscriber, &s, 16));
			continue;
		}

		free(s);
		while (reader(subscriber, &s, 16));

		while(1) {
			chat_id = 0, position = 0;
			while (s[position] <= '0' || s[position] >= '9') position++;
			while(1) {
				chat_id = chat_id * 10 + (s[position] - '0');
				if (s[++position] == ',') break;
				else if (s[position] == '\0') break;
			}
			contacts[level][index] = chat_id;
			contacts[level] = realloc(contacts[level], (++index + 1) * sizeof(int));
			if (s[position] == ',') {
				free(s);
				while ((code = reader(subscriber, &s, 16)));
				continue;
			}
			else if (s[position] == '\0') {
				free(s);
				while (reader(subscriber, &s, 16));
				break;
			}
		}

		indexes[level] = index;
		level++;
		free(s);
		while (reader(subscriber, &s, 16));
	}
	free(s);
	fclose(subscriber);
	return EXIT_SUCCESS;
}

int main(void) {

	contacts = malloc(13 * sizeof(int *));
	for (int i = 0; i < 13; i++) {
		contacts[i] = malloc(sizeof(int));
	}
	indexes = malloc(13 * sizeof(int));

	contacts_parser();

	for (int i = 0; i < 13; i++) {

		printf("Magnitude %d, %d subscribers", i, indexes[i]);
		if (indexes[i]) {
			printf("\n\t");
			for (int j = 0; j < indexes[i]; j++) {
				printf("%d ", contacts[i][j]);
			}
		}
		puts("");
	}

	/*if (curl_global_init(CURL_GLOBAL_ALL)) {	// Init the library
		fprintf(stderr, "Boh non è partito\n");
		exit(EXIT_FAILURE);
	}
	CURL *easyhandle;
	if (easyhandle = curl_easy_init()) {	// Init a handle which will be used for every connection
		fprintf(stderr, "Boh non è partito easy\n");
		exit(EXIT_FAILURE);
	}

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	size_t bufsz = snprintf(NULL, 0, "http://webservices.ingv.it/fdsnws/event/1/query?starttime=%d-%d-%dT%d%%3A00%%3A00&endtime=%d-%d-%dT%d%%3A59%%3A59&maxmag=20&orderby=time-asc&format=text&limit=15000", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour - 1, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
	char *URL = malloc((bufsz + 1) * sizeof(char));
	sprintf(URL, "http://webservices.ingv.it/fdsnws/event/1/query?starttime=%d-%d-%dT%d%%3A00%%3A00&endtime=%d-%d-%dT%d%%3A59%%3A59&maxmag=20&orderby=time-asc&format=text&limit=15000", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour - 1, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);

	curl_easy_setopt(easyhandle, CURLOPT_URL, "http://domain.com/");

	puts("Cleanup");
	curl_easy_cleanup(easyhandle);
	curl_global_cleanup();*/

	free(indexes);
	for (int i = 0; i < 13; i++) {
		free(contacts[i]);
	}
	free(contacts);

	return 0;
}