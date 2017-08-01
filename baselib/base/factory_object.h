#ifndef BASE_FACTORY_OBJECT_H_
#define BASE_FACTORY_OBJECT_H_

#include <string>
#include <memory>

template< typename T > class FactoryObject {
public:
    virtual ~FactoryObject() {}

	virtual const std::string& GetType() const = 0;
    virtual std::shared_ptr<T> CreateInstance() const = 0;
};

#endif
