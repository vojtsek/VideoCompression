#ifndef TEMPLATES_H
#define TEMPLATES_H

template <typename Measure_inT = std::chrono::milliseconds>
struct Measured {
    template <typename FuncT, typename ... args>
    static typename Measure_inT::rep exec_measure(FuncT func, args && ... arguments) {
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        func(std::forward<args>(arguments) ...);
        Measure_inT duration = std::chrono::duration_cast<Measure_inT>
                                    (std::chrono::system_clock::now() - start);
        return duration.count();
    }
};

template <typename T>
void applyToVector(std::vector<T *> vec, void (*op)(T *)) {
    std::for_each(vec.begin(), vec.end(), op);
}

template <typename T, typename S>
void applyToMap(std::map<T, S *> map, void (*op)(S *)) {
    std::for_each(map.begin(), map.end(),
                  [&](std::pair<T, S *> p) { op(p.second);
                                          });
}

template<typename T>
int32_t sendSth(T what, int32_t fd) {
    int32_t w;
    if ((w = write(fd, &what, sizeof (T))) != sizeof(T)) {
        reportDebug("Problem occured while sending the data.", 1);
        return (-1);
    }
    return (w);
}

template<typename T>
int32_t recvSth(T &where, int32_t fd) {
    int32_t r;
     if ((r = read(fd, &where, sizeof (T))) != sizeof(T)) {
        reportDebug("Problem occured while accepting the data.", 1);
        return (-1);
    }
    return (r);
}

#endif // TEMPLATES_H
