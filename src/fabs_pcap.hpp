#ifndef FABS_PCAP_HPP
#define FABS_PCAP_HPP

#include "fabs_callback.hpp"
#include "fabs_bytes.hpp"

#include <pcap/pcap.h>

#include <stdint.h>

#include <string>
#include <list>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

class fabs_pcap {
public:
    fabs_pcap(std::string conf);

    virtual ~fabs_pcap() {
        if (m_handle != NULL)
            pcap_close(m_handle);
    }

    void set_dev(std::string dev);

    void callback(const struct pcap_pkthdr *h, const uint8_t *bytes);
    
    void consume();
    void timer();

    void run();
    void stop() { m_is_break = true; }

private:
    std::string m_dev;
    pcap_t *m_handle;
    int     m_dl_type;
    bool    m_is_break;

    const uint8_t *get_ip_hdr(const uint8_t *bytes, uint32_t len,
                              uint8_t &proto);

    fabs_callback m_callback;

    std::list<fabs_bytes> m_queue;
    boost::mutex  m_mutex;
    boost::condition m_condition;
    boost::thread m_thread_consume;
    boost::thread m_thread_timer;
};

extern boost::shared_ptr<fabs_pcap> pcap_inst;
extern bool pcap_is_running;

void stop_pcap();

void run_pcap(std::string dev, std::string conf);

#endif // FABS_PCAP_HPP
