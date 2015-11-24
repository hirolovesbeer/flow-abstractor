#ifndef FABS_PCAP_HPP
#define FABS_PCAP_HPP

#include "fabs_common.hpp"
#include "fabs_ether.hpp"
#include "fabs_dlcap.hpp"

#include <pcap/pcap.h>

#include <stdint.h>

#include <string>
#include <list>

#include <boost/shared_array.hpp>


class fabs_pcap : public fabs_dlcap {
public:
    fabs_pcap(std::string conf);
    virtual ~fabs_pcap();

    void set_dev(std::string dev);
    void set_bufsize(int size);

    void callback(const struct pcap_pkthdr *h, const uint8_t *bytes);
    
    void run();
    void stop() { m_is_break = true; }

    virtual void print_stat() const;

private:
    fabs_ether m_ether;

    std::string m_dev;
    pcap_t *m_handle;
    volatile bool m_is_break;
    int     m_dl_type;
    int     m_bufsize;
};

extern boost::shared_ptr<fabs_pcap> pcap_inst;
extern bool pcap_is_running;

void stop_pcap();

void run_pcap(std::string dev, std::string conf, int bufsize);

#endif // FABS_PCAP_HPP
