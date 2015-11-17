#ifndef FABS_APPIF_HPP
#define FABS_APPIF_HPP

#include "fabs_common.hpp"
#include "fabs_id.hpp"
#include "fabs_spin_rwlock.hpp"
#include "fabs_cb.hpp"

#include <event.h>

#include <sys/time.h>

#include <list>
#include <map>
#include <set>
#include <string>
#include <deque>

#include <re2/re2.h>

//#include <boost/regex.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

enum fabs_stream_event {
    // abstraction events
    STREAM_CREATED   = 0,
    STREAM_DESTROYED = 1,
    STREAM_DATA      = 2,

    // primitive event
    STREAM_SYN,
    STREAM_FIN,
    STREAM_TIMEOUT,
    STREAM_RST,
};

static const int DATAGRAM_DATA = STREAM_DATA; // SYNONYM

class fabs_callback;
class fabs_tcp;

class fabs_appif {
public:
    fabs_appif();
    virtual ~fabs_appif();

    void read_conf(std::string conf);
    void run();

    void in_event(fabs_stream_event st_event,
                  const fabs_id_dir &id_dir, ptr_fabs_bytes bytes);

    void print_info();

    int  get_tcp_timeout() const { return m_tcp_timeout; }
    int  get_num_tcp_threads() const { return m_num_tcp_threads; }

private:
    typedef boost::shared_ptr<RE2> ptr_regex;
    typedef boost::shared_ptr<boost::filesystem::path> ptr_path;

    enum ifproto {
        IF_UDP,
        IF_TCP,
        IF_OTHER
    };

    enum ifformat {
        IF_BINARY,
        IF_TEXT
    };

    struct ifrule {
        ptr_regex   m_up, m_down;
        std::string m_name;
        ifproto     m_proto;
        ifformat    m_format;
        ptr_path    m_ux;
        bool        m_is_body;
        int         m_nice;
        int         m_balance;
        std::vector<std::string>   m_balance_name;
        std::map<int, std::string> m_fd2name; // listen socket to name
        boost::shared_ptr<std::list<std::pair<uint16_t, uint16_t> > > m_port;

        ifrule() : m_proto(IF_OTHER), m_format(IF_TEXT), m_is_body(true),
                   m_nice(100), m_balance(1),
                   m_port(new std::list<std::pair<uint16_t, uint16_t> >) { }
    };

    struct uxpeer {
        int          m_fd;
        event       *m_ev;
        bool         m_is_avail;
        std::string  m_name;
    };

    typedef boost::shared_ptr<ifrule>        ptr_ifrule;

    enum match_dir {
        MATCH_UP   = 0,
        MATCH_DOWN = 1,
        MATCH_NONE = 2,
    };

    struct loopback_state {
        bool is_header;
        fabs_appif_header header;
        fabs_id_dir id_dir;
        std::set<fabs_id> streams;

        loopback_state() : is_header(true) {
        }
    };

    struct stream_info {
        ptr_ifrule m_ifrule;
        timeval    m_create_time;
        uint64_t   m_dsize1, m_dsize2;
        bool       m_is_created;         // sent created event?
        bool       m_is_giveup;
        bool       m_is_buf1, m_is_buf2; // recv data?
        std::deque<ptr_fabs_bytes> m_buf1, m_buf2;
        uint32_t   m_hash;
        match_dir  m_match_dir[2];
        fabs_appif_header m_header;

        void clear_buf();

        stream_info(const fabs_id &id);
        virtual ~stream_info();
    };

    struct ifrule_storage {
        std::list<ptr_ifrule> ifrule;
        std::list<ptr_ifrule> ifrule_no_regex;
    };

    typedef boost::shared_ptr<uxpeer>         ptr_uxpeer;
    typedef boost::shared_ptr<boost::thread>  ptr_thread;
    typedef boost::shared_ptr<loopback_state> ptr_loopback_state;
    typedef boost::shared_ptr<stream_info>    ptr_info;
    typedef boost::shared_ptr<ifrule_storage> ptr_ifrule_storage;

    struct appif_event {
        fabs_stream_event st_event;
        fabs_id_dir       id_dir;
        ptr_fabs_bytes    bytes;
    };

    struct ifrule_storage2 {
        std::list<ptr_ifrule> ifrule;
        std::list<ptr_ifrule> ifrule_no_regex;
        ptr_ifrule cache_up[256];
        ptr_ifrule cache_down[256];
    };

    typedef boost::shared_ptr<ifrule_storage2> ptr_ifrule_storage2;

public:
    class appif_consumer {
    public:
        appif_consumer(int id, fabs_appif &appif);
        virtual ~appif_consumer();

        void produce(appif_event *ev);
        void consume();
        void run();

    private:
        int              m_id;
        bool             m_is_break;
        bool             m_is_consuming;
        fabs_appif      &m_appif;
        boost::mutex     m_mutex;
        boost::condition m_condition;
        boost::thread    m_thread;
        fabs_cb<appif_event*> m_ev_queue;
        std::map<fabs_id, ptr_info> m_info;
        std::map<int, ptr_ifrule_storage2> m_ifrule_tcp;
        std::map<int, ptr_ifrule_storage2> m_ifrule_udp;

        void in_stream_event(fabs_stream_event st_event,
                             const fabs_id_dir &id_dir, ptr_fabs_bytes bytes);
        bool send_tcp_data(ptr_info p_info, fabs_id_dir id_dir);
        void in_datagram(const fabs_id_dir &id_dir, ptr_fabs_bytes bytes);

        friend class fabs_appif;
    };
private:

    boost::mutex     m_mutex_init;
    boost::condition m_condition_init;

    typedef boost::shared_ptr<appif_consumer> ptr_consumer;

    int m_fd7;
    int m_fd3;

    std::map<int, ptr_loopback_state> m_lb7_state;
    ifformat m_lb7_format;

    std::map<int, ptr_ifrule_storage> m_ifrule_tcp;
    std::map<int, ptr_ifrule_storage> m_ifrule_udp;
    ptr_ifrule m_ifrule7;
    ptr_ifrule m_ifrule3;
    ptr_ifrule m_tcp_default;
    ptr_ifrule m_udp_default;
    std::map<int, ptr_ifrule> m_fd2ifrule; // listen socket
    std::map<int, ptr_uxpeer> m_fd2uxpeer; // accepted socket
    std::map<std::string, std::set<int> > m_name2uxpeer;

    spin_rwlock m_rw_mutex;

    int m_num_tcp_threads;
    int m_num_consumer;
    boost::shared_array<ptr_consumer> m_consumer;

    ptr_thread  m_thread_listen;

    event_base *m_ev_base;
    ptr_path    m_home;

    bool        m_is_lru;
    bool        m_is_cache;

    int         m_tcp_timeout;

    void makedir(boost::filesystem::path path);
    bool write_event(int fd, const fabs_id_dir &id_dir, ptr_ifrule ifrule,
                     fabs_stream_event event, match_dir match,
                     fabs_appif_header *header, char *body, int bodylen);
    void ux_listen();
    void ux_listen_ifrule(ptr_ifrule ifrule);
    bool is_in_port(boost::shared_ptr<std::list<std::pair<uint16_t, uint16_t> > > range,
                    uint16_t port1, uint16_t port2);

    friend void ux_accept(int fd, short events, void *arg);
    friend void ux_read(int fd, short events, void *arg);
    friend void ux_close(int fd, fabs_appif *appif);
    friend bool read_loopback7(int fd, fabs_appif *appif);
//    friend bool read_loopback3(int fd, fabs_appif *appif);
};

typedef boost::shared_ptr<fabs_appif> ptr_fabs_appif;

#endif // FABS_APPIF_HPP
