#ifndef TEMPLATES_H
#define TEMPLATES_H

/*!
 * \struct Measured wrapps passed function
 * Calls the function and measures time of the execution
 */
template <typename Measure_inT = std::chrono::milliseconds>
struct Measured {
    template <typename FuncT, typename ... args>
    static typename Measure_inT::rep exec_measure(FuncT func, args && ... arguments) {
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        int64_t ret = func(std::forward<args>(arguments) ...);
        Measure_inT duration = std::chrono::duration_cast<Measure_inT>
                                    (std::chrono::system_clock::now() - start);
        if (ret != -1) {
                    return duration.count();
        } else {
                    return ret;
        }
    }
};

/*!
 * sends unspecified data over the given file descriptor
 * does not handle byte order
 */
template<typename T>
int64_t sendSth(T what, int64_t fd) {
    int64_t w;
    // should write corresponding size
    if ((w = write(fd, &what, sizeof (T))) != sizeof(T)) {
        reportDebug("Problem occured while sending the data.", 1);
        return (-1);
    }
    return (w);
}

/*!
 * recvSth receives unspecfied data from the given file descriptor
 * does not handle byte order
 */
template<typename T>
int64_t recvSth(T &where, int64_t fd) {
    int64_t r;
    // should read corresponding size
     if ((r = read(fd, &where, sizeof (T))) != sizeof(T)) {
        reportDebug("Problem occured while accepting the data.", 1);
        return (-1);
    }
    return (r);
}

#endif // TEMPLATES_H
