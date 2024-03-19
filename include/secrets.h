// WiFi Configuration
#define WIFI_SSID "<wifi_ssid>"            // <= Edit WiFi SSID.
#define WIFI_PASSWORD "<wifi_password>"  // <= Edit WiFi password.


// Thing topic
#define THING_GROUP "<yourGroup>"
#define AWS_IOT_PUB_TOPIC "<pub_topic>"
#define AWS_IOT_SUB_TOPIC "<sub_topic>"

// Amazon Account endpoint
#define AWS_IOT_ENDPOINT "<uniquID>.iot.<region>.amazonaws.com" // <= Edit AWS IoT Endpoint.

// Amazon Certificate
extern const char AWS_CERT_CA[] asm("_binary_certs_ca_pem_start");
extern const char AWS_CERT_CRT[] asm("_binary_certs_cert_pem_start");
extern const char AWS_CERT_PRIVATE[] asm("_binary_certs_private_pem_start");
