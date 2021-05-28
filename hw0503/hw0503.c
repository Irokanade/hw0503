//
//  hw0503.c
//  hw0503
//
//  Created by michaelleong on 25/05/2021.
//

#include <stdio.h>
#include <curl/curl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define headHtml "<cite class=\"data\" itemprop=\"headline\">"
#define authorHtml "<span itemprop=\"name\" title=\""
#define titleHtml "<span class=\"title\" itemprop=\"name\">"
#define dateHtml "<span itemprop=\"datePublished\">"
#define sourceHtml "<span itemprop=\"name\">"

#define strBuff 257

struct option long_options[] = {
    {"query", 1, NULL, 'q'},
    { 0, 0, 0, 0},
};

typedef struct _memory {
    char *memory;
    size_t size;
    
} memory;

static size_t writecallback(void *contents, size_t size, size_t nmemb, void *userp) {
    //fprintf(stderr, "Got %lu bytes\n", size*nmemb);
    
    size_t realsize = size*nmemb;
    memory *mem = (memory*)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void query(char *data);
int searchMember(char *data, char *html, char *endStr, char *result);

int main(int argc, char *argv[]) {
    int c = 0;
    int getoptIndex = 0;
    int queryOption = 0;
    
    char queryArg[strBuff] = {0};
    
    while ( ( c = getopt_long( argc, argv, "q:", long_options, &getoptIndex ) ) != -1 )
    {
        //printf( "index: %d\n", index );
        switch( c )
        {
            case 'q':
                queryOption = 1;
                strncpy(queryArg, optarg, strlen(optarg));
                break;
            case '?':
                printf("invalid option >:[\n");
                break;
            default:
                printf("invalid option >:[\n");
                break;
        }
    }
    
    if(queryOption) {
        CURL *curl;
        CURLcode res;
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        memory chunk = {NULL, 0};
        
        char url[strBuff] = "https://dblp.org/search?q=";
        strncat(url, queryArg, strlen(queryArg));
        
        printf("url: %s\n", url);
        
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writecallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
            
            res = curl_easy_perform(curl);
            
            if(res != CURLE_OK) {
                fprintf(stderr, "curl_easy_perform() returned %s\n", curl_easy_strerror(res));
            } else {
                //printf("We got %lu bytes to our callback in memory %p\n", chunk.size, chunk.memory);
                //printf("%s", chunk.memory);
                //call query
                query(chunk.memory);
            }
            
            free(chunk.memory);
            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
    }
    
    return 0;
}

void query(char *data) {
    char *parserPtr = NULL;
    
    parserPtr = strstr(data, headHtml);
    uint8_t noOfArticles = 0;
    
    if(parserPtr == NULL) {
        printf("No matches\n");
        return;
    }
    
    while(parserPtr != NULL && noOfArticles < 10) {
        parserPtr += strlen(headHtml);
        
        char *endOfArticle = NULL;
        endOfArticle = strstr(parserPtr, headHtml);
        
        if(endOfArticle != NULL) {
            *(endOfArticle-1) = 0;
        }
        
        //Get authors
        char author[100][strBuff] = {0};
        char *ptr = NULL;
        ptr = strstr(parserPtr, authorHtml);
        char *token = NULL;
        int32_t authorIndex = 0;
        
        if(ptr == NULL) {
            strncpy(author[0], "Cannot find author", strlen("Cannot find author"));
            authorIndex = 1;
        } else {
            while(ptr != NULL) {
                ptr += strlen(authorHtml);
                token = strtok(ptr, "\"");
                strncpy(author[authorIndex], token, strlen(token));
                ptr = token+strlen(token)+1;
                *(ptr-1) = '\"';
                ptr = strstr(ptr, authorHtml);
                authorIndex++;
            }
        }
        
        ptr = NULL;
        token = NULL;
        
        //Get title
        char title[strBuff] = {0};
        ptr = strstr(parserPtr, titleHtml);
        if(ptr == NULL) {
            strncpy(title, "Cannot find title", strlen("Cannot find title"));
        } else {
            ptr += strlen(titleHtml);
            token = strtok(ptr, "<");
            strncpy(title, token, strlen(token));
            *(token+strlen(token)) = '<';
            ptr = NULL;
            token = NULL;
        }
        
        //Get date
        char date[strBuff] = {0};
        ptr = strstr(parserPtr, dateHtml);
        if(ptr == NULL) {
            strncpy(date, "Cannot find date", strlen("Cannot find date"));
        } else {
            *(ptr-1) = 0;
            ptr += strlen(dateHtml);
            token = strtok(ptr, "<");
            strncpy(date, token, strlen(token));
        }
        
        
        //Get source
        char source[strBuff] = {0};
        ptr = strstr(parserPtr, sourceHtml);
        if(ptr == NULL) {
            strncpy(source, "Cannot find source", strlen("Cannot find source"));
        } else {
            ptr += strlen(sourceHtml);
            token = strtok(ptr, "<");
            while(token != NULL) {
                strncat(source, token, strlen(token));
                token = strtok(NULL, "<");
                if(token == NULL) {
                    break;
                }
                token = strchr(token, '>');
                if(token != NULL) {
                    token++;
                }
            }
        }
        
        //print articles
        printf("Paper %2u\n", noOfArticles + 1);
        printf("\tTitle : %s\n", title);
        printf("\tAuthor : ");
        for (int32_t i = 0; i < authorIndex; i++) {
            printf("%s", author[i]);
            if (i != authorIndex - 1)
                    printf(", ");
        }
        printf("\n\tSource : %s\n", source);
        printf("\tDate : %s\n", date);
        //printf("%s\n", parserPtr);
        noOfArticles++;
        if (endOfArticle == NULL)
            break;
        else
            parserPtr = strstr(endOfArticle, headHtml);
    }
}

int searchMember(char *data, char *html, char *endStr, char *result) {
    char *resultTemp = NULL;
    
    resultTemp = strstr(data, html);
    
    if(resultTemp == NULL) {
        //search failed return 0
        return 0;
    }
    
    //printf("resultTemp: %s\n", resultTemp);
    
    //char *token = strtok(resultTemp, endStr);
    
    resultTemp += strlen(html);
    
    //printf("resultTemp: %s", resultTemp);
    
    size_t offset = strcspn(resultTemp, endStr);
        
    strncpy(result, resultTemp, offset);
    result[offset] = 0;
    
    //printf("result: %s\n", result);
    //successfully found
    return 1;
}
