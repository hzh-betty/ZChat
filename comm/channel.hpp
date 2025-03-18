#pragma once
#include <brpc/channel.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "logger.hpp"

namespace zchat
{
    /*封装单个服务的信道管理类*/
    class ServiceChannel
    {
    public:
        using ptr = std::shared_ptr<ServiceChannel>;
        using ChannelPtr = std::shared_ptr<brpc::Channel>;
        ServiceChannel(const std::string &name) : serviceName_(name), index_(0) {}

        // 服务上线了一个节点，则调用append新增信道
        void append(const std::string &host)
        {
            // 1. 配置channel信息
            auto channel = std::make_shared<brpc::Channel>();
            brpc::ChannelOptions options;
            options.connect_timeout_ms = -1;
            options.timeout_ms = -1; // 连接等待超时时间
            options.max_retry = 3;   // rpc请求等待超时时间
            options.protocol = "baidu_std";

            // 2. 初始化
            int ret = channel->Init(host.c_str(), &options);
            if (ret == -1)
            {
                LOG_ERROR("初始化{}-{}信道失败!", serviceName_, host);
                return;
            }

            // 3. 插入管理
            std::unique_lock<std::mutex> lock(mutex_);
            hosts_.insert(std::make_pair(host, channel));
            channels_.push_back(channel);
        }

        // 服务下线了一个节点，则调用remove释放信道
        void remove(const std::string &host)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = hosts_.find(host);
            if (it == hosts_.end())
            {
                LOG_WARN("{}-{}节点删除信道时，没有找到信道信息！", serviceName_, host);
                return;
            }

            for (auto vit = channels_.begin(); vit != channels_.end(); ++vit)
            {
                if (*vit == it->second)
                {
                    channels_.erase(vit);
                    break;
                }
            }
            hosts_.erase(it);
        }

        // 通过RR轮转策略，获取一个Channel用于发起对应服务的Rpc调用
        ChannelPtr choose()
        {
            size_t sz = channels_.size();
            if (sz == 0)
            {
                LOG_ERROR("当前没有能够提供 {} 服务的节点！", serviceName_);
                return ChannelPtr();
            }
            index_++;
            int32_t idx = index_ % sz;
            return channels_[idx];
        }

    private:
        std::mutex mutex_;
        std::atomic<int32_t> index_;                        // 当前轮转下标计数器
        std::string serviceName_;                           // 服务名称
        std::vector<ChannelPtr> channels_;                  // 当前服务对应的信道集合
        std::unordered_map<std::string, ChannelPtr> hosts_; // 主机地址与信道映射关系
    };

    /*总体的服务信道管理类*/
    class ServiceManager
    {
    public:
        using ptr = std::shared_ptr<ServiceManager>;
        ServiceManager() = default;

        // 获取指定服务的节点信道
        ServiceChannel::ChannelPtr choose(const std::string &service_name)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            auto sit = services_.find(service_name);
            if (sit == services_.end())
            {
                LOG_ERROR("当前没有能够提供 {} 服务的节点！", service_name);
                return ServiceChannel::ChannelPtr();
            }
            return sit->second->choose();
        }

        // 先声明，我关注哪些服务的上下线，不关心的就不需要管理了
        void declared(const std::string &service_name)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            followServices_.insert(service_name);
        }

        // 服务上线时调用的回调接口，将服务节点管理起来
        void onServiceOnline(const std::string &service_instance, const std::string &host)
        {
            std::string service_name = getServiceName(service_instance);
            ServiceChannel::ptr service;
            {
                // 1. 先判断当前服务是否需要关心
                std::unique_lock<std::mutex> lock(mutex_);
                auto fit = followServices_.find(service_name);
                if (fit == followServices_.end())
                {
                    LOG_DEBUG("{}-{} 服务上线了，但是当前并不关心！", service_name, host);
                    return;
                }

                // 2. 获取管理对象，没有则创建，有则添加节点
                auto sit = services_.find(service_name);
                if (sit == services_.end())
                {
                    service = std::make_shared<ServiceChannel>(service_name);
                    services_.insert(std::make_pair(service_name, service));
                }
                else
                {
                    service = sit->second;
                }
            }

            if (!service)
            {
                LOG_ERROR("新增 {} 服务管理节点失败！", service_name);
                return;
            }
            service->append(host);
            LOG_DEBUG("{}-{} 服务上线新节点，进行添加管理！", service_name, host);
        }

        // 服务下线时调用的回调接口，从服务信道管理中，删除指定节点信道
        void onServiceOffline(const std::string &service_instance, const std::string &host)
        {
            std::string service_name = getServiceName(service_instance);
            ServiceChannel::ptr service;
            {
                // 1. 先判断当前服务是否需要关心
                std::unique_lock<std::mutex> lock(mutex_);
                auto fit = followServices_.find(service_name);
                if (fit == followServices_.end())
                {
                    LOG_DEBUG("{}-{} 服务下线了，但是当前并不关心！", service_name, host);
                    return;
                }

                // 2. 删除节点
                auto sit = services_.find(service_name);
                if (sit == services_.end())
                {
                    LOG_WARN("删除{}服务节点时，没有找到管理对象", service_name);
                    return;
                }
                service = sit->second;
            }
            service->remove(host);
            LOG_DEBUG("{}-{} 服务下线节点，进行删除管理！", service_name, host);
        }

    private:
        std::string getServiceName(const std::string &service_instance)
        {
            auto pos = service_instance.find_last_of('/');
            if (pos == std::string::npos)
                return service_instance;
            return service_instance.substr(0, pos);
        }

    private:
        std::mutex mutex_;
        std::unordered_set<std::string> followServices_; // 需要关注的服务节点名称
        std::unordered_map<std::string, ServiceChannel::ptr> services_;
    };
}