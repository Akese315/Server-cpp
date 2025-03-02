#pragma once
#include <stdio.h>
#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <arpa/inet.h>
#include <shared_mutex>
#include "Logger.hpp"

struct Datacache
{
    const static ushort COMPLETED = 0;
    const static ushort PENDING = 1;
    const static ushort NOT_USE = 2;
    char *data;
    ushort dataLen;
    ushort state = Datacache::NOT_USE;
    ushort event;
};

template <typename T = Datacache>
class Client
{
public:
    bool isActive;

    Client(uint32_t adress, ushort port, int socket)
    {
        this->isActive = true;
        this->address = address;
        this->port = port;
        this->socket = socket;
        this->currentCache = new T();
    }
    ~Client()
    {
        // condition variable here before closing
        // check if tasks is == 0
        close(this->socket);
    }

    int getSocket()
    {
        return this->socket;
    }
    std::string get_adress_str()
    {
        struct in_addr addr{};
        addr.s_addr = this->address;
        std::string adress = std::string(inet_ntoa(addr)) + ":" + std::to_string(this->port);
        return adress;
    }
    uint32_t get_ip()
    {
        return this->address;
    }
    ushort get_port()
    {
        return this->port;
    }

    void set_active(bool active)
    {
        this->isActive = active;
    }

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
            }
        }
        else
        {

            std::string error = std::string(strerror(errno)) + std::string(" in ") + std::string(where);
        }
    };

    void sendData(const char *data, size_t len)
    {
        try
        {
            if (!this->isActive)
            {
                Logger::add_logs("Client is not active");
                return;
            }
            std::unique_lock<std::shared_mutex> lock(this->fdmutex);
            this->onSendData(data, len);
        }
        catch (...)
        {
            Logger::add_logs("Error while sending", LogLevel::ERROR);
            throw std::runtime_error("Error while sending");
        }
    };
    int receiveData(void *data, size_t len)
    {
        std::shared_lock<std::shared_mutex> lock(this->fdmutex);
        return onReceiveData(data, len);
    }

private:
    uint32_t address;
    ushort port;
    ushort tasks;
    int socket;
    std::shared_mutex fdmutex;
    T *previousCache;
    T *currentCache;

protected:
    virtual int onReceiveData(void *data, size_t len) // do not call directly this function
    {
        int bytes = recv(this->socket, data, len, MSG_DONTWAIT);
        return bytes;
    }

    virtual void onSendData(const char *data, size_t len)
    {
        int bytes = send(this->socket, data, len, MSG_DONTWAIT); // do not call directly this function
        if ((size_t)bytes != len)
        {
            get_socket_error("Send data", this->socket);
        }
    }
};