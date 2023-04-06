#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <pthread.h>
#include <vector>

#define err_exit(code, str) { std::cerr << str << ": " << strerror(code) \
  << std::endl; exit(EXIT_FAILURE); }

char c_response[] = "HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n";

struct data_client {
  int client_fd;
  int request_number;
};
char recieve[500];
pthread_mutex_t mutex;
pthread_mutex_t mutex2;
pthread_cond_t cond;
pthread_cond_t cond2;

int done =  0;
std::vector<data_client> queries; //запросы

void *thread_job(void *arg){
  int err;
  while(true) {
    // Захватываем мьютекс и ждем новый запрос для пула потоков
    err = pthread_mutex_lock(&mutex);
    if(err != 0)
      err_exit(err, "Cannot lock mutex");
    while(queries.empty()) {
      err = pthread_cond_wait(&cond, &mutex);
      if(err != 0)
        err_exit(err, "Cannot wait on condition variable");
    }
    // Получен сигнал, что в пуле появилось новое задание, берем запрос из очереди
    data_client params = queries.back();
    queries.pop_back();

    int client_fd= params.client_fd;
    int number = params.request_number;

    read(client_fd, &recieve, 500);
    std::string response = c_response;
    response += "Request number: " + std::to_string(number) + '\n';
    write(client_fd, response.c_str(), response.size() * sizeof(char));

    close(client_fd);
    done++;
    printf("request %i has been done\n", done);
      
    // Открываем мьютекс
    err = pthread_cond_signal(&cond2);
    err = pthread_mutex_unlock(&mutex);
    if(err != 0)
      err_exit(err, "Cannot unlock mutex");
  	}
}

int main(int argc, char *argv[])
{
  int threads_count = atoi(argv[1]);
  pthread_t* threads = new pthread_t[threads_count];
  int count = 0;
  
  int one = 1, client_fd;
  struct sockaddr_in svr_addr, cli_addr;
  socklen_t sin_len = sizeof(cli_addr);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    err(1, "can't open socket");

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

  int port = 8080;
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = INADDR_ANY;
  svr_addr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
    close(sock);
    err(1, "Can't bind");
  }
  
  int error;
  // Инициализируем мьютекс
  error = pthread_mutex_init(&mutex, NULL);
  error = pthread_mutex_init(&mutex2, NULL);
  if(error != 0)
    err_exit(error, "Cannot initialize mutex");
  
  // Инициализируем условную переменную
  error = pthread_cond_init(&cond, NULL);
  error = pthread_cond_init(&cond2, NULL);
  if(error != 0)
    err_exit(error, "Cannot initialize condition variable");

  //создаем потоки и делаем их отсоединенными
  for (int i = 0; i < threads_count; ++i){
    error = pthread_create(&threads[i], NULL, thread_job, NULL);
    if(error != 0) {
      std::cout << "Cannot create a thread: " << strerror(error) << std::endl;
      exit(-1);
    }
    pthread_detach(threads[i]);
  }
  
  listen(sock, 5);
  data_client params;
  printf("server started at %i port...\n", port);
  while (true) {
    error = pthread_mutex_lock(&mutex2);
    while (queries.size()>threads_count) {
        error = pthread_cond_wait(&cond2, &mutex2);
        if(error != 0)
        err_exit(error, "Cannot wait on condition variable");
    }
      
    count++;
    client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);

    if (client_fd == -1) {
      perror("Can't accept\n");
      continue;
    }
    
    //готовим данные для передачи в функцию
    params.client_fd  = client_fd;
    params.request_number = count;
    queries.push_back(params);

    // Посылаем сигнал, что появился запрос в очереди
    error = pthread_cond_signal(&cond);
    if(error != 0)
      err_exit(error, "Cannot send signal");
    error = pthread_mutex_unlock(&mutex2);
    if(error != 0)
      err_exit(error, "Cannot unlock mutex");
  }

	return 0;
}
