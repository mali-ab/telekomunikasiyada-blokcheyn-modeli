#pragma once

#include "smartContract.h"

#include <string>
#include <vector>

class SubscriberRepository {
public:
    static int count();
    static std::vector<Subscriber> list();
    static std::vector<std::string> names();
};
