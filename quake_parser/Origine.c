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
#include <useful.h>

#include <curl/curl.h>

typedef struct MemoryStruct {
	char *memory;
	size_t size;
	int res;
} MemoryStruct;

int **contacts;
int *indexes;
CURL *easyhandle;

void end_free(void) {
	puts("Cleanup");

	curl_easy_cleanup(easyhandle);
	curl_global_cleanup();

	free(indexes);
	for (int i = 0; i < 13; i++) {
		free(contacts[i]);
	}
	free(contacts);
}

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

		while (1) {
			chat_id = 0, position = 0;
			while (s[position] <= '0' || s[position] >= '9') position++;
			while (1) {
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

void print_subscribers(void) {
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
}

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL) {
		/* out of memory! */
		printf("Not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

MemoryStruct quakes_getter(void) {

	printf("Init... ");

	if (curl_global_init(CURL_GLOBAL_ALL)) {	// Init the library
		fprintf(stderr, "Boh non è partito\n");
		exit(EXIT_FAILURE);
	}
	if (!(easyhandle = curl_easy_init())) {	// Init a handle which will be used for every connection
		fprintf(stderr, "Boh non è partito easy\n");
		exit(EXIT_FAILURE);
	}

	puts("Handle initiated");

	time_t t = time(NULL);
	struct tm endtime = *localtime(&t);
	struct tm starttime = endtime;
	starttime.tm_hour--;
	//remove!
	starttime.tm_mday--;
	mktime(&starttime);
	printf("Start time: %d/%d/%d - %d:%d:%d\n", starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, starttime.tm_hour, starttime.tm_min, starttime.tm_sec);
	size_t bufsz = snprintf(NULL, 0, "http://webservices.ingv.it/fdsnws/event/1/query?starttime=%d-%02d-%02dT%02d%%3A00%%3A00&endtime=%d-%02d-%02dT%02d%%3A59%%3A59&maxmag=20&orderby=time-asc&format=text&limit=15000", starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, starttime.tm_hour, endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday, endtime.tm_hour);
	char *URL = malloc((bufsz + 1) * sizeof(char));
	sprintf(URL, "http://webservices.ingv.it/fdsnws/event/1/query?starttime=%d-%02d-%02dT%02d%%3A00%%3A00&endtime=%d-%02d-%02dT%02d%%3A59%%3A59&maxmag=20&orderby=time-asc&format=text&limit=15000", starttime.tm_year + 1900, starttime.tm_mon + 1, starttime.tm_mday, starttime.tm_hour, endtime.tm_year + 1900, endtime.tm_mon + 1, endtime.tm_mday, endtime.tm_hour);

	curl_easy_setopt(easyhandle, CURLOPT_URL, URL);

	//printf("URL set: %s\n", URL);

	MemoryStruct chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, (void *)&chunk);

	chunk.res = curl_easy_perform(easyhandle);

	return chunk;

}

void url_encoder(char **original) {
	int i = 0, j = 0, count = 0;
	while ((*original)[i] != '\0') {
		if ((*original)[i] == ' '|| (*original)[i] == ':' || (*original)[i] == '/' || (*original)[i] == ',' || (*original)[i] == '[' || (*original)[i] == ']') count++;
		else if ((*original)[i] == '\n') count--;
		i++;
	}
	char *newstring = malloc((strlen(*original) + 2 * count + 1) * sizeof(char));
	i = 0;
	while ((*original)[i] != '\0') {
		if ((*original)[i] == ' ') {
			i++;
			newstring[j] = '%';
			j++;
			newstring[j] = '2';
			j++;
			newstring[j] = '0';
			j++;
		}
		else if ((*original)[i] == ':') {
			i++;
			newstring[j] = '%';
			j++;
			newstring[j] = '3';
			j++;
			newstring[j] = 'A';
			j++;
		}
		else if ((*original)[i] == '/') {
			i++;
			newstring[j] = '%';
			j++;
			newstring[j] = '2';
			j++;
			newstring[j] = 'F';
			j++;
		}
		else if ((*original)[i] == ',') {
			i++;
			newstring[j] = '%';
			j++;
			newstring[j] = '2';
			j++;
			newstring[j] = 'C';
			j++;
		}
		else if ((*original)[i] == '[') {
			i++;
			newstring[j] = '%';
			j++;
			newstring[j] = '5';
			j++;
			newstring[j] = 'B';
			j++;
		}
		else if ((*original)[i] == ']') {
			i++;
			newstring[j] = '%';
			j++;
			newstring[j] = '5';
			j++;
			newstring[j] = 'D';
			j++;
		}
		else if ((*original)[i] == '\n') {
			i++;
		}
		else {
			newstring[j] = (*original)[i];
			i++;
			j++;
		}
	}
	free(*original);
	*original = newstring;
}

int quake_parser(MemoryStruct quakes) {
	for (int i = 0; i <= quakes.size; i++) {
		if (quakes.memory[i] == '\n') quakes.memory[i] = '\0';
	}
	char *s = quakes.memory;
	s += (strlen(s) + 1);
	MemoryStruct chunk;
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, (void *)&chunk);

	while (strlen(s)) {
		int year, month, day, hour, minute;
		char *location = calloc(100, sizeof(char));
		float magnitude;
		sscanf(s, "%*d|%d-%d-%dT%d:%d:%*f|%*f|%*f|%*f|%*[^||||]||||%*[^|]|%f|--|%[^\t\n]", \
			&year, &month, &day, &hour, &minute, &magnitude, location);
		s += (strlen(s) + 1);

		// TODO: Send messages to contacts, grouped by magnitude
		/*if ((int)magnitude == 6) {

			size_t bufsz = snprintf(NULL, 0, "%d/%d/%d at %d:%02d: magnitude %.1f, %s\n", year, month, day, hour, minute, magnitude, location);
			char *text = malloc((bufsz + 1) * sizeof(char));
			sprintf(text, "%d/%d/%d at %d:%02d: magnitude %.1f, %s\n", year, month, day, hour, minute, magnitude, location);
			printf("Entry:\n%s", text);

			url_encoder(&text);

			bufsz = snprintf(NULL, 0, "%s%s%s%s", "https://api.telegram.org/bot", TOKEN, "/sendMessage?chat_id=66441008&text=", text);
			char *URL = malloc((bufsz + 1) * sizeof(char));
			sprintf(URL, "%s%s%s%s", "https://api.telegram.org/bot", TOKEN, "/sendMessage?chat_id=66441008&text=", text);
			curl_easy_setopt(easyhandle, CURLOPT_URL, URL);
			//printf("URL set: %s", URL);

			chunk.memory = malloc(1);
			chunk.size = 0;
			//curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			//curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, (void *)&chunk);
			//curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1);

			chunk.res = curl_easy_perform(easyhandle);
			if (chunk.res == CURLE_OK) puts("Sent.\n");
			else printf("Returned with errorcode %d\n", chunk.res);
			free(text);
			free(URL);
			free(chunk.memory);

		}*/

		size_t bufsz = snprintf(NULL, 0, "%d/%d/%d alle %d:%02d: magnitudine %.1f, %s\n", year, month, day, hour, minute, magnitude, location);
		char *text = malloc((bufsz + 1) * sizeof(char));
		sprintf(text, "%d/%d/%d alle %d:%02d: magnitudine %.1f, %s\n", year, month, day, hour, minute, magnitude, location);
		printf("Entry:\n%s", text);

		url_encoder(&text);


		for (int i = ((int)magnitude < 13) ? (int)magnitude : 12; i >= 0; i--) {
			printf("Index at %d\n", i);
			for (int j = 0; j < indexes[i]; j++) {

				bufsz = snprintf(NULL, 0, "%s%s%s%d%s%s", "https://api.telegram.org/bot", TOKEN, "/sendMessage?chat_id=", contacts[i][j], "&text=", text);
				char *URL = malloc((bufsz + 1) * sizeof(char));
				sprintf(URL, "%s%s%s%d%s%s", "https://api.telegram.org/bot", TOKEN, "/sendMessage?chat_id=", contacts[i][j], "&text=", text);
				curl_easy_setopt(easyhandle, CURLOPT_URL, URL);
				printf("Sending to %d, %d... ", contacts[i][j], j);

				chunk.memory = malloc(1);
				chunk.size = 0;

				//chunk.res = curl_easy_perform(easyhandle);
				chunk.res = CURLE_OK;
				if (chunk.res == CURLE_OK) puts("Sent.\n");
				else printf("Returned with errorcode %d\n", chunk.res);
				free(URL);
				free(chunk.memory);
				getchar();
			}
		}
		free(text);
	}

	return EXIT_SUCCESS;
}

int main(void) {

#ifndef TOKEN
	fprintf(stderr, "No TOKEN specified!\n");
	return EXIT_FAILURE;
#endif // !TOKEN


	contacts = malloc(13 * sizeof(int *));
	for (int i = 0; i < 13; i++) {
		contacts[i] = malloc(sizeof(int));
	}
	indexes = malloc(13 * sizeof(int));

	if (contacts_parser() == EXIT_FAILURE) {
		fprintf(stderr, "Failed to parse subscribers\n");
		end_free();
		return 0;
	}
	//print_subscribers();

	MemoryStruct quakes = quakes_getter();

	//printf("libcURL returned with code %d\n", quakes.res);
	//printf("%lu bytes retrieved\n", (unsigned long)quakes.size);

	quake_parser(quakes);

	end_free();

	return 0;
}