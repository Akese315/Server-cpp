#pragma once
#include <condition_variable>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <set>
#include <vector>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <csignal>
#include <shared_mutex>
#include <memory>
#include "Supervisor.hpp"
#include "ProcessMonitor.hpp"
#include "Client.hpp"

struct Listener
{
    int socket;
    unsigned short type;
    int timeout;
    unsigned short port;
};

template <typename T = Client<Datacache>>
class Server : public Supervisor
{

public:
    Server(std::string name) : Supervisor(name)
    {
        loop_bool = true;
        is_running = true;

        if (pipe(pipe_fd) == -1)
        {
            get_error("pipe");
        }
    };
    ~Server()
    {
        close_all_connections();
        if (is_running)
        {
            stop();
        }
    };

    void ban_client(T &client)
    {
        interrupt_epoll_wait(Server::BAN, client.get_socket());
    };

    void disconnect_client(T &client)
    {
        interrupt_epoll_wait(Server::HOST_DISCONNECTION, client.get_socket());
    };

    void restart()
    {
        if (is_running)
        {
            stop();
        }
        loop_bool = true;
        is_running = true;
        my_thread = std::thread(&Server<T>::loop, this);
    };

    void broadcast_message(const char *data, size_t len)
    {
        for (auto pair : socket_client_map)
        {
            pair->second->send_data(data, len);
        }
    };

    std::shared_ptr<T> find_client(int socket)
    {
        auto it = socket_client_map.find(socket);
        if (it == socket_client_map.end())
        {
            return nullptr;
        }
        else
        {
            return it->second;
        }
    }

    std::shared_ptr<T> *find_ban(uint32_t addr)
    {
        auto it = addr_ban_set.find(addr);
        if (it == addr_ban_set.end())
        {
            return nullptr;
        }
        else
        {
            return &it->second;
        }
    }

    void stop()
    {
        is_running = false;
        interrupt_epoll_wait(Server::STOP, 0);
        Logger::add_logs("Server is stopping...", LogLevel::WARNING, false);
        if (my_thread.joinable())
        {
            my_thread.join();
        }
        Logger::add_logs("Server is stopped", LogLevel::WARNING, false);
    };

    virtual void receive_task(std::shared_ptr<T> client, uint32_t task_flag) {};

    virtual void on_disconnect(std::shared_ptr<T> client, uint32_t reason) {};

    void start()
    {
        Logger::add_logs("Server is starting...", LogLevel::PASS, false);
        bool can_start = init();
        if (!can_start)
        {
            Logger::add_logs("Can't start.", LogLevel::ERROR);
            return;
        }
        Logger::add_logs("Server started :)", LogLevel::PASS);

        for (size_t i = 0; i < listener_array.size(); i++)
        {
            listen(listener_array[i].socket, 5);
        }
        my_thread = std::thread(&Server<T>::loop, this);
    };

    Listener create_listener(uint16_t type, uint16_t port, uint16_t timeout)
    {
        Listener listener = {};
        listener.port = port;
        listener.type = type;
        listener.timeout = timeout;

        if (type == Server<T>::TCP)
        {
            listener.socket = socket(AF_INET, SOCK_STREAM, 0);
        }
        else if (type == Server<T>::UDP)
        {
            listener.socket = socket(AF_INET, SOCK_DGRAM, 0);
        }
        if (listener.socket == -1 || listener.socket == 0)
        {
            get_socket_error("create listener", listener.socket);
        }
        Logger::add_logs("socket created : " + std::to_string(listener.socket), LogLevel::INFO);
        return listener;
    };

    int interrupt_epoll_wait(uint32_t flag, int fd)
    {
        uint32_t message = 0;
        encode_interrupt_wait_message(message, flag, fd);
        std::unique_lock lock(pipe_unique_mutex);
        if (write(pipe_fd[1], &message, 4) == -1)
        {
            perror("write");
            return -1;
        }
        return 0;
    };

    void add_listener(Listener listener)
    {
        listener_array.push_back(listener);
    };

    void remove_listener(int position)
    {
        listener_array.erase(listener_array.begin() + position);
    };

    const static uint16_t TCP = 1;
    const static uint16_t LISTENER = 1;
    const static uint16_t UDP = 2;

    const static uint16_t CONNECTION = 3;
    const static uint16_t CLIENT_DISCONNECTION = 4;
    const static uint16_t HOST_DISCONNECTION = 5;
    const static uint16_t TIMEOUT = 6;
    const static uint16_t UNEXPECTED_DISCONNECTION = 7;
    const static uint16_t DATA = 8;
    const static uint16_t BAN = 9;
    const static uint16_t STOP = 10;

    const static uint16_t MAX_EVENTS_SIZE = 1023;
    const static uint16_t MAX_USERS = 65535;

private:
    struct epoll_event events[MAX_EVENTS_SIZE];
    std::vector<Listener> listener_array;
    std::set<uint32_t, T *> addr_ban_set;
    std::unordered_map<int, std::shared_ptr<T>> socket_client_map;
    int active_events = 0;
    int epoll_fd;
    int pipe_fd[2];
    std::vector<int> fds;

    std::mutex loop_bool_mutex;
    std::mutex pipe_unique_mutex;
    std::thread my_thread;
    bool loop_bool;
    bool is_running;

    int keepalive = 1;
    int keepidle = 60;
    int keepintvl = 10;
    int keepcnt = 5;

    void get_socket_error(const char *where, int fd)
    {
        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0)
        {
            if (err != 0) // Only handle actual errors
            {
                std::string error = std::string("Socket error: ") + std::string(strerror(err));
                Logger::add_logs(error, LogLevel::ERROR);
                disconnect(fd);
            }
        }
        else
        {
            get_error("Failed to get socket error status");
        }
    };

    void get_error(const char *where)
    {
        std::string error = std::string(strerror(errno)) + std::string(" in ") + std::string(where);
    }

    bool has_event(uint32_t events)
    {
        return (events == 0) ? false : true;
    };

    bool is_pipe(int fd)
    {
        return (fd == pipe_fd[0]) ? true : false;
    };

    bool is_error_event(uint32_t events)
    {
        return (events & EPOLLERR) ? true : false;
    };

    bool is_read_event(uint32_t events)
    {
        return (events == EPOLLIN || events == (EPOLLIN | EPOLLET));
    }

    bool is_listener(int fd)
    {
        for (size_t j = 0; j < listener_array.size(); j++)
        {
            if (listener_array[j].socket != fd)
            {
                continue;
            }
            if (listener_array[j].type != Server<T>::TCP)
            {
                continue;
            }
            return true;
        }
        return false;
    }

    bool check_file_descriptor(int fdr)
    {
        ProcessMonitor pm("check_file_descriptor");

        for (int i = 0; i < active_events && fdr > 0; i++)
        {
            int fd = events[i].data.fd;
            if (!is_read_event(events[i].events))
            {
                get_socket_error("revents", fd);
                continue;
            }
            if (!has_event(events[i].events))
            {
                continue;
            }
            if (is_pipe(fd))
            {
                if (!process_pipe_message(fd))
                {
                    return false;
                }
                continue;
            }
            if (is_listener(fd))
            {
                new_connection(fd);
                fdr--;
                continue;
            }
            if (is_timeout(fd))
            {
                disconnect(fd, Server<T>::TIMEOUT);
                // send_task(std::string("server"), std::string("server"), Server<T>::TIMEOUT, fd);
            }
            if (is_unexpected_disconnection(fd))
            {
                disconnect(fd, Server<T>::UNEXPECTED_DISCONNECTION);
                // send_task(std::string("server"), std::string("server"), Server<T>::UNEXPECTED_DISCONNECTION, fd);
            }

            if (has_client_disconnected(fd))
            {
                disconnect(fd, Server<T>::CLIENT_DISCONNECTION);
                // send_task(std::string("server"), std::string("server"), Server<T>::DISCONNECTED, fd);
            }
            else
            {
                send_task(std::string("server"), std::string("server"), Server<T>::DATA, fd);
            }
            fdr--;
        }
        return true;
    };

    bool process_pipe_message(uint32_t fd)
    {
        uint32_t message;
        uint16_t flag, fd_check;
        if (read(fd, &message, 4) == -1)
        {
            get_socket_error("read", fd);
            return false;
        }
        decode_interrupt_wait_message(message, flag, fd_check);
        Logger::add_logs("Received a message from the pipe, flag :" + std::to_string(flag) + "fd : " + std::to_string(fd_check));
        if (flag == Server<T>::STOP)
        {
            Logger::add_logs("Received a stop message", LogLevel::CRITICAL, true);
            return false;
        }
        if (flag == Server<T>::BAN)
        {
            // to do
        }
        if (flag == Server<T>::HOST_DISCONNECTION)
        {
            disconnect(fd_check);
        }
        return true;
    }

    void disconnect(int fd, int reason = Server<T>::CLIENT_DISCONNECTION)
    {
        std::shared_ptr<T> client = find_client(fd);
        if (client == nullptr)
        {
            Logger::add_logs("Client has already been disconnected", LogLevel::WARNING);
            return;
        }
        client->set_active(false);
        on_disconnect(client, reason);
        socket_client_map.erase(fd);
        remove_file_descriptor(fd);
        fds.erase(std::remove(fds.begin(), fds.end(), fd), fds.end());
        close(fd);
    };

    bool is_timeout(int fd)
    {
        char buffer[1024];
        ssize_t bytes_received = recv(fd, buffer, sizeof(buffer), MSG_PEEK);
        if (bytes_received < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return true;
            }
        };
        return false;
    };

    bool is_unexpected_disconnection(int fd)
    {
        char buffer[1024];
        ssize_t bytes_received = recv(fd, buffer, sizeof(buffer), MSG_PEEK);
        if (bytes_received < 0)
        {
            if (errno == ECONNRESET || errno == ETIMEDOUT || errno == ENOTCONN)
            {
                return true;
            }
        }
        return false;
    };

    bool has_client_disconnected(int fd)
    {
        char buffer[1024];
        ssize_t byte = recv(fd, buffer, 1, MSG_PEEK);
        if (byte == 0)
        {
            return true;
        }
        if (byte == -1)
        {
            if (errno == ECONNRESET || errno == ETIMEDOUT || errno == ENOTCONN)
            {
                // Déconnexion brutale ou timeout
                return true;
            }
        }
        return false;
    };

    void encode_interrupt_wait_message(uint32_t &message, uint16_t flag, uint16_t fd)
    {
        message |= flag;
        message <<= 16;
        message |= fd;
    }

    void decode_interrupt_wait_message(uint32_t message, uint16_t &flag, uint16_t &fd)
    {
        fd = message & 0xFFFF;
        message >>= 16;
        flag = message & 0xFFFF;
    }

    int new_connection(int fd)
    {
        struct sockaddr_in socket_param{};
        socklen_t socket_addr_len = sizeof(socket_param);
        int new_fd = accept(fd, (struct sockaddr *)&socket_param, (socklen_t *)&socket_addr_len);
        if (new_fd < 0)
        {
            Logger::add_logs("error on accept socket", LogLevel::ERROR);
            get_socket_error("accept", fd);
            return -1;
        }
        if (setsockopt(new_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) == -1)
        {
            Logger::add_logs(std::string("Erreur : SO_KEEPALIVE - ") + std::string(strerror(errno)), LogLevel::ERROR);
        }
        // define TCP_KEEPIDLE
        if (setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) == -1)
        {
            Logger::add_logs(std::string("Erreur : TCP_KEEPIDLE - ") + std::string(strerror(errno)), LogLevel::ERROR);
        }
        // define TCP_KEEPINTVL
        if (setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) == -1)
        {
            Logger::add_logs(std::string("Erreur : TCP_KEEPINTVL - ") + std::string(strerror(errno)), LogLevel::ERROR);
        }
        // define TCP_KEEPCNT
        if (setsockopt(new_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt)) == -1)
        {
            Logger::add_logs(std::string("Erreur : TCP_KEEPCNT - ") + std::string(strerror(errno)), LogLevel::ERROR);
        }

        add_fd(new_fd);

        std::shared_ptr<T> client = std::make_shared<T>(socket_param.sin_addr.s_addr, socket_param.sin_port, new_fd);
        socket_client_map.insert(std::pair<int, std::shared_ptr<T>>(new_fd, client));
        send_task(std::string("server"), std::string("server"), Server<T>::CONNECTION, new_fd);
        return new_fd;
    };

    void add_fd(int fd)
    {
        if (active_events == MAX_EVENTS_SIZE)
        {
            return;
        }

        events[active_events].data.fd = fd;
        events[active_events].events = EPOLLIN | EPOLLET;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &events[active_events]);
        fds.push_back(fd);
        active_events++;
    };

    void remove_file_descriptor(int fd)
    {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
        {
            get_socket_error("D fd: ", fd);
        }
        for (int i = 0; i < active_events; i++)
        {
            if (events[i].data.fd == fd)
            {
                memmove(&events[i], &events[i + 1], (active_events - i - 1) * sizeof(epoll_event));
                active_events -= 1;
            }
        }
    }

    void close_all_connections()
    {
        for (unsigned short i = 1; i < active_events; i++)
        {
            close(events[i].data.fd);
        }

        close(pipe_fd[0]); // close reading pipe
        close(pipe_fd[1]); // close writing pipe
    };

    bool init()
    {
        if (listener_array.empty())
        {
            Logger::add_logs("No listeners set", LogLevel::ERROR);
            return false;
        }

        epoll_fd = epoll_create1(0);
        if (epoll_fd == -1)
        {
            get_socket_error("epol", epoll_fd);
        }

        if (fcntl(pipe_fd[0], F_SETFL, O_NONBLOCK) == -1)
        {
            get_socket_error("fcntl", pipe_fd[0]);
        }

        fds.push_back(pipe_fd[0]); // add read pipe to fds

        events[0].events = EPOLLIN | EPOLLET;
        events[0].data.fd = pipe_fd[0];
        Logger::add_logs("pipe created : " + std::to_string(pipe_fd[0]), LogLevel::INFO);

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fd[0], &events[0]) == -1)
        {
            get_socket_error("epoll_ctl", pipe_fd[0]);
        }

        int on = 1;
        for (long unsigned int i = 0; i < listener_array.size(); i++)
        {
            setsockopt(listener_array[i].socket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, (socklen_t)sizeof(on));
            fcntl(listener_array[i].socket, F_SETFL, O_NONBLOCK);
            events[i + 1].events = EPOLLIN | EPOLLET;
            events[i + 1].data.fd = listener_array[i].socket;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listener_array[i].socket, &events[i + 1]) == -1)
            {
                get_socket_error("Epoll ctl", listener_array[i].socket);
            }
            active_events++;
        }

        for (long unsigned int i = 0; i < listener_array.size(); i++)
        {
            sockaddr_in bind_params;
            bind_params.sin_family = AF_INET;
            bind_params.sin_port = htons(listener_array[i].port);
            inet_aton("127.0.0.1", &bind_params.sin_addr);
            int error = bind(listener_array[i].socket, (struct sockaddr *)&bind_params, (socklen_t)sizeof(bind_params));

            if (error != 0)
            {
                if (errno == EADDRINUSE)
                {
                    Logger::add_logs("Port " + std::to_string(listener_array[i].port) + " is already in use", LogLevel::ERROR, false);
                }
                get_socket_error("bind", listener_array[i].socket);
                return false;
            }

            std::string info = "server bind on port : " + std::to_string(listener_array[i].port);
            Logger::add_logs(info, LogLevel::INFO, false);
        }
        return true;
    };

    std::string get_flag_string(uint16_t flag)
    {
        switch (flag)
        {
        case Server<T>::CONNECTION:
            return "CONNECTION";
        case Server<T>::CLIENT_DISCONNECTION:
            return "CLIENT_DISCONNECTION";
        case Server<T>::HOST_DISCONNECTION:
            return "HOST_DISCONNECTION";
        case Server<T>::TIMEOUT:
            return "TIMEOUT";
        case Server<T>::UNEXPECTED_DISCONNECTION:
            return "UNEXPECTED_DISCONNECTION";
        case Server<T>::DATA:
            return "MESSAGE";
        case Server<T>::BAN:
            return "BAN";
        case Server<T>::STOP:
            return "STOP";
        default:
            return "UNKNOWN";
        }
    }

    void async_function(Task task) override
    {
        auto it = socket_client_map.find(task.fd);
        if (it == socket_client_map.end())
        {
            Logger::add_logs("Client not found in map", LogLevel::ERROR);
            return;
        }
        std::shared_ptr<T> client = it->second;
        receive_task(client, task.flag);
    };

    void send_task(std::string destination, std::string source, int flag, int fd)
    {
        Task task{};
        task.destination = destination;
        task.source = source;
        task.flag = flag;
        task.fd = fd;
        do_task(task);
    };

    void loop()
    {
        Logger::add_logs("Listening...", LogLevel::INFO);
        do
        {
            int fdr = epoll_wait(epoll_fd, events, MAX_EVENTS_SIZE, -1);
            if (fdr < 0)
            {
                if (fdr == -1)
                {
                    if (errno == EINTR)
                    {
                        Logger::add_logs("Intereupted system call", LogLevel::WARNING);
                        continue;
                    }
                }
                perror("Erreur dans epoll_wait()");
                get_error("epoll_wait");
                break;
            }
            if (fdr == 0)
            {
                Logger::add_logs("Time out reached", LogLevel::WARNING);
                break;
            }
            if (fdr > 0)
            {
                if (!check_file_descriptor(fdr))
                {
                    Logger::add_logs("Stop has been triggered", LogLevel::CRITICAL);
                    break;
                }
            }
        } while (true);
        Logger::add_logs("not Listening...", LogLevel::INFO);
    };
};
