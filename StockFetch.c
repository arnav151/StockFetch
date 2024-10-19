#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h> // Include the json-c header

#define API_KEY "INSERT APLHA VANTAGE API KEY"
#define API_URL "https://www.alphavantage.co/query"

struct string {
    char *ptr;
    size_t len;
};

// Function to initialize the string struct
void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

// Callback function for curl to write the response data
size_t write_callback(void *ptr, size_t size, size_t nmemb, struct string *s) {
    size_t new_len = s->len + size * nmemb;
    s->ptr = realloc(s->ptr, new_len + 1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        return 0;
    }
    memcpy(s->ptr + s->len, ptr, size * nmemb);
    s->ptr[new_len] = '\0'; // Null-terminate the string
    s->len = new_len;

    return size * nmemb;
}

// Function to parse JSON response and print user-friendly output
void printStockData(const char *jsonResponse) {
    struct json_object *parsed_json;
    struct json_object *meta_data, *time_series, *latest_entry, *latest_timestamp;
    const char *latest_time;

    parsed_json = json_tokener_parse(jsonResponse);
    if (parsed_json == NULL) {
        fprintf(stderr, "Error parsing JSON response\n");
        return;
    }

    // Access the "Meta Data" section
    if (json_object_object_get_ex(parsed_json, "Meta Data", &meta_data) &&
        json_object_object_get_ex(parsed_json, "Time Series (1min)", &time_series)) {

        // Get the latest timestamp
        latest_time = json_object_get_string(json_object_object_get(meta_data, "3. Last Refreshed"));

        // Access the latest entry
        latest_entry = json_object_object_get(time_series, latest_time);

        // Check if the latest entry exists
        if (latest_entry) {
            printf("Stock Data for %s:\n", json_object_get_string(json_object_object_get(meta_data, "2. Symbol")));
            printf("Last Refreshed: %s\n", latest_time);
            printf("Current Price: $%s\n", json_object_get_string(json_object_object_get(latest_entry, "4. close")));
            printf("Open Price: $%s\n", json_object_get_string(json_object_object_get(latest_entry, "1. open")));
            printf("High Price: $%s\n", json_object_get_string(json_object_object_get(latest_entry, "2. high")));
            printf("Low Price: $%s\n", json_object_get_string(json_object_object_get(latest_entry, "3. low")));
            printf("Volume: %s shares\n", json_object_get_string(json_object_object_get(latest_entry, "5. volume")));
        } else {
            fprintf(stderr, "No data available for the specified time\n");
        }
    } else {
        fprintf(stderr, "Error retrieving data from the response\n");
    }

    // Clean up
    json_object_put(parsed_json);
}

void fetch_stock_data(const char *symbol) {
    CURL *curl;
    CURLcode res;
    struct string response;

    init_string(&response);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        // Prepare the URL with the API call
        char url[256];
        snprintf(url, sizeof(url), "%s?function=TIME_SERIES_INTRADAY&symbol=%s&interval=1min&apikey=%s", API_URL, symbol, API_KEY);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        // Perform the request
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Call the function to print user-friendly output
            printStockData(response.ptr);
        }

        // Clean up
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Failed to initialize CURL\n");
    }

    free(response.ptr);
    curl_global_cleanup();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <stock_symbol>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fetch_stock_data(argv[1]);

    return EXIT_SUCCESS;
}
