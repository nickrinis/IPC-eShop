#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>

//Defining constant variables.
#define PORT 8080
#define PRODUCT_COUNT 20
#define ORDER_COUNT 10
#define CLIENT_COUNT 5

//Every product is a struct.
typedef struct product{

    int ID;
    char* description;
    int item_count;
    float price;
    int request_count;
    int sold_count;
}product;

char* product_descr[20] = { "Percy Jackson and the Olympians: The lightning thief", "Percy Jackson and the Olympians: The Sea of Monsters", "Percy Jackson and the Olympians: The Titans Curse", "Percy Jackson and the Olympians: The Battle of the Labyrinth", "Percy Jackson and the Olympians: The Last Olympian", 
                          "The Lord of the Rings: The fellowship of the Ring", "The Lord of the Rings: The Two Towers", "The Lord of the Rings: The Return of the King", "The Hobbit", "A Game of Thrones", 
                          "A Clash of Kings", "A Storm of Swords", "A Feast for Crows", "A Dance with Dragons", "The Winds of Winter",
                          "Around the World in Eighty Days", "From the Earth to the Moon", "From the Earth to the Moon", "Twenty Thousand Leagues Under the Sea", "City of Bones"};

int unsatisfied_clients[20][5] = {0};
int total_orders = 0, total_successful_orders = 0, total_unsuccessful_orders = 0;
float total_profit = 0.0;

int check_avail(product*);
void print_product_details(product*);

int main(int argc, char** argv){
	
    //Declarations for the inter-process communication using sockets:
    struct sockaddr_in address;
    struct sockaddr_in serv_address;
    int addrlen = sizeof(address);
    int opt = 1;
    int server_sock, client_sock = 0, msg_sock, s_orders[10];
    int w_flag, r_flag;

    //Declarations:
    srand(time(NULL));
    int i = 0, j, k, l, stop_forking = 0, status = 0, is_avail;
    pid_t pid, wpid;
    char update_message[1024];
    char euro_sign[] = "€";
    uint32_t id_sent, prdct_ID_sent, id_received, prdct_ID_received, id_converted, prdct_ID_converted;

    while(i < CLIENT_COUNT && !stop_forking){

        //Creating the clients.
        pid = fork();
        //Client process.
        if(pid == 0){
            
            //Waiting for the server to create the socket first.
            sleep(3);

            //Declaring the seed to be different for each client process by taking the pid into account.
            time_t t;
            srand((int)time(&t) % getpid());
            int client_orders[10];

            //Getting product requests.
            for(j=0; j<ORDER_COUNT; j++){
                client_orders[j] = rand()%20;
            }

            if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("[ERROR - CLIENT]: socket()\n");
                exit(-1); 
            }

            serv_address.sin_family = AF_INET;
            serv_address.sin_port = htons(PORT);

            //Convert IPv4 and IPv6 addresses from text to binary form.
            if (inet_pton(AF_INET, "127.0.0.1", &serv_address.sin_addr) <=0 ) {
                perror("[ERROR - CLIENT]: invlaid address\n");
                return -1; 
            }

            //Connecting to the server.
            while (connect(client_sock, (struct sockaddr *)&serv_address, sizeof(serv_address)) < 0){
                
                if (errno == ENOENT) {
                    sleep(1); continue; 
                }
                else {
                    perror("[ERROR - CLIENT]: connect()\n");;
                    exit(-2); 
                }    
            }

            //Sending the order to the server.
            w_flag = write(client_sock, client_orders, 10 * sizeof(int));
            if (w_flag == -1){
             
                perror("[ERROR - CLIENT]: write()\n");
                exit(-3); 
            }

            for(l = 0; l < ORDER_COUNT; l++){

                //Reading the update from the server for each product.
                r_flag = read(client_sock, &update_message, 1024);
                if (r_flag == -1) {

                    perror("[ERROR - SERVER]: read()\n");
                    exit(-4); 
                }
                //Printing the update to the terminal.
                printf("\n[CLIENT %d]: Product: %d, %s\n", getpid(), client_orders[l], update_message);

                //Checking availability and adjusting struct info
                char not_avail[] = "Order was unsuccessful...";
                if (!strcmp(update_message, not_avail)){
                    
                    //Converting the numbers to be sent
                    id_sent = htonl(i);
                    prdct_ID_sent = htonl(client_orders[l]);

                    sleep(1);
                    //Sending the user ID to the server.
                    w_flag = write(client_sock, &id_sent, sizeof(id_sent));
                    if (w_flag == -1){
                    
                        perror("[ERROR - CLIENT]: write()\n");
                        exit(-5); 
                    }
                    //Sending the product ID to the server.
                    w_flag = write(client_sock, &prdct_ID_sent, sizeof(prdct_ID_sent));
                    if (w_flag == -1){
                    
                        perror("[ERROR - CLIENT]: write()\n");
                        exit(-6); 
                    }
                }
            }

            sleep(1);
            close(client_sock);

            //Changing the flag so the child process won't create children of it's own.
            stop_forking = 1;
        }
        else if (pid == -1){
            //Print error message in case the fork fails.
            perror("[ERROR]: Error while forking.\n");
        }
        i++;
    }

    //Server code.
    if (pid != 0){

        //Catalog declaration and initialization.
        product catalog[PRODUCT_COUNT];
        for (i = 0; i < PRODUCT_COUNT; i++){
            //Assinging increasing ID number per product as well as a description, the item count, price (random between 1 and 10) and the request and sold counters as 0.
            catalog[i].ID = i;
            catalog[i].description = product_descr[i];
            catalog[i].item_count = 2;
            catalog[i].price = ((float)rand()/(float)(RAND_MAX) * 10) + 1;
            catalog[i].request_count = 0;
            catalog[i].sold_count = 0;
        }
        
        //Creating the socket for local communication.
        if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0){
            perror("[ERROR - SERVER]: socket()\n");
            exit (-1);
        }
        
        if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
            perror("[ERROR - SERVER]: setsockopt()\n");
            exit (-2); 
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        //Binding address to the socket.
        if (bind(server_sock, (struct sockaddr *)&address, sizeof(address)) < 0){
            perror("[ERROR - SERVER]: bind()\n");
            exit(-3); 
        }

        //Listening for a maximum of 5 requests.
        if (listen(server_sock, 5) < 0) {
            perror("[ERROR - SERVER]: listen()\n");
            exit(-4);
        }

        //For each client.
        for (k=0; k<CLIENT_COUNT; k++){

            //Accepting from client.
            if ((msg_sock = accept(server_sock, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("[ERROR - SERVER]: accept()\n");
                exit(-5);
            }
            //Reading the data from the client.
            r_flag = read(msg_sock, s_orders, 10 * sizeof(int));
            if (r_flag == -1) {

                perror("[ERROR - SERVER]: read()\n");
                exit(-6); 
            }
            //Waiting for 0.5 seconds.
            sleep(0.5);

            for(l = 0; l < ORDER_COUNT; l++){
                
                total_orders++;
                catalog[s_orders[l]].request_count++;

                //Calling function to check the availability.
                is_avail = check_avail(&catalog[s_orders[l]]);
                if (is_avail == 1){
                    
                    //Increasing the necessary counters.
                    total_successful_orders++;
                    catalog[s_orders[l]].sold_count++;
                    total_profit += catalog[s_orders[l]].price;

                    char updt_msg[] = "Order was successful! | Cost: ";
                    //Getting the cost and converting it in order to concatenate it.
                    char price_buffer[1024];
                    float tmp_price = catalog[s_orders[l]].price;
                    sprintf(price_buffer, "%.2f", tmp_price);
                    strcat(updt_msg, price_buffer);
                    strcat(updt_msg, euro_sign);

                    //Writing the successful order message in the socket.
                    w_flag = write(msg_sock, updt_msg, strlen(updt_msg)+1);
                    if (w_flag == -1){
                        
                        perror("[ERROR - CLIENT]: write()\n");
                        exit(-7); 
                    }
                }
                else{
                    
                    total_unsuccessful_orders++;

                    char updt_msg[] = "Order was unsuccessful...";
                    w_flag = write(msg_sock, updt_msg, strlen(updt_msg)+1);
                    if (w_flag == -1){
                        
                        perror("[ERROR - CLIENT]: write()\n");
                        exit(-7); 
                    }

                    //Reading the ID of the unsatisfied client.
                    r_flag = read(msg_sock, &id_received, sizeof(id_received));
                    if (r_flag == -1) {

                        perror("[ERROR - SERVER]: read()\n");
                    exit(-8); 
                    }
                    //Reading the product ID for the unsatisfied client.
                    r_flag = read(msg_sock, &prdct_ID_received, sizeof(prdct_ID_received));
                    if (r_flag == -1) {

                        perror("[ERROR - SERVER]: read()\n");
                    exit(-8); 
                    }

                    //Converting back the received numbers and adding the unsatisfied client to the correct place in the array.
                    id_converted = ntohl(id_received);
                    int tmp = id_converted + 1;
                    prdct_ID_converted = ntohl(prdct_ID_received);
                    unsatisfied_clients[prdct_ID_converted][id_converted] = tmp;
                }
            }
            //Waiting for 1 second before the next request.
            sleep(1);
        }
        close(server_sock);
        close(msg_sock);

        //Printing the data per product after all the client processes stopped and then the final report.
        while ((wpid = wait(&status)) > 0);
        printf("\n\nInformation per product after all the client orders finished:\n\n");
        for(int i=0; i<PRODUCT_COUNT; i++){
            
            //Calling function for struct info.
            print_product_details(&catalog[i]);
            printf("Clients that didn't receive the product:");
            for (int clnt = 0; clnt < CLIENT_COUNT; clnt++){
                printf(" %d ", unsatisfied_clients[i][clnt]);
            }
            printf("\n");
        }
        printf("*************************************************************************************\n");

        printf("\n~FINAL REPORT~\n");
        printf("Total order requests for products: %d\n", total_orders);
        printf("Total successful order requests: %d\n", total_successful_orders);
        printf("Total unsuccessful order requests: %d\n", total_unsuccessful_orders);
        printf("Total income: %.2f€\n\n", total_profit);
    }
    return 0;
}

int check_avail(product * curr_product){

    sleep(0.5);
    //Check if there are available products.
    if(curr_product->item_count==0){
        return 0;
    }
    //If there are available products, give one to the client.
    curr_product->item_count--;
    return 1;
}

//Function to print struct details.
void print_product_details(product * product_details){

    printf("*************************************************************************************\n");
    printf("~Product ID: %d | Title: %s~\n", product_details->ID, product_details->description);
    printf("Number of order requests for the product: %d\n", product_details->request_count);
    printf("Number of times the product got sold: %d\n", product_details->sold_count);
}