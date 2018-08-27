#include "base/configuration.h"

folly::dynamic Configuration::GetValue(const std::string& k) const {
    folly::dynamic rv = nullptr;
    
    if (dynamic_conf_.isArray()) {
        for (auto& v2 : dynamic_conf_) {
            // confs
            if (!v2.isObject()) continue;
            if (0 == v2.count(k)) continue;
            rv = v2[k];
            break;
        }
    } else {
        auto tmp = dynamic_conf_.get_ptr(k);
        if (tmp) {
            rv = *tmp;
        }
    }
    
    return rv;
}


