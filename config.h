//WiFi AP information
#define SSID          "your_wireless_network"
#define PASS          "wireless_network_password"

//Grafana/HTTP options
#define HTTP_CONTEXT  "/metrics/job/powermeter/instance/home"    //you may change this context
#define HOSTNAME      "pushgateway.prometheus.localhost"
#define HTTP_PORT     "2491"

//When to update server? After 25 blinks
#define SERVER_UPDATE_THRESHOLD 25

//Comment to turn off Debug mode
#define DEBUG
