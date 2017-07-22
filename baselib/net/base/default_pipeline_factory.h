#ifndef NET_BASE_DEFAULT_PIPELINE_FACTORY_H_
#define NET_BASE_DEFAULT_PIPELINE_FACTORY_H_

#include <wangle/channel/Pipeline.h>

template <typename Pipeline>
class DefaultPipelineFactory : public wangle::PipelineFactory<Pipeline> {
public:
    virtual ~DefaultPipelineFactory() = default;
    
    virtual std::string GetTypeName() const = 0;
};


#endif
