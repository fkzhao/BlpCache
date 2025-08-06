#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <fstream>


#define DECLARE_FIELD(FIELD_TYPE, FIELD_NAME) extern FIELD_TYPE FIELD_NAME

#define DECLARE_Bool(name) DECLARE_FIELD(bool, name)
#define DECLARE_Int16(name) DECLARE_FIELD(int16_t, name)
#define DECLARE_Int32(name) DECLARE_FIELD(int32_t, name)
#define DECLARE_Int64(name) DECLARE_FIELD(int64_t, name)
#define DECLARE_Double(name) DECLARE_FIELD(double, name)
#define DECLARE_String(name) DECLARE_FIELD(std::string, name)

#define DEFINE_FIELD(FIELD_TYPE, FIELD_NAME, FIELD_DEFAULT, VALMUTABLE)                      \
FIELD_TYPE FIELD_NAME;                                                                   \
static Register reg_##FIELD_NAME(#FIELD_TYPE, #FIELD_NAME, &(FIELD_NAME), FIELD_DEFAULT, \
VALMUTABLE);

#define DEFINE_Bool(name, defaultstr) DEFINE_FIELD(bool, name, defaultstr, false)
#define DEFINE_Double(name, defaultstr) DEFINE_FIELD(double, name, defaultstr, false)
#define DEFINE_Int16(name, defaultstr) DEFINE_FIELD(int16_t, name, defaultstr, false)
#define DEFINE_Int32(name, defaultstr) DEFINE_FIELD(int32_t, name, defaultstr, false)
#define DEFINE_Int64(name, defaultstr) DEFINE_FIELD(int64_t, name, defaultstr, false)
#define DEFINE_String(name, defaultstr) DEFINE_FIELD(std::string, name, defaultstr, false)


namespace blp {
    namespace config {
        DECLARE_Int16(port);
        DECLARE_Int16(replica_port);
        DECLARE_String(replica_host);


        extern std::mutex custom_conf_lock;

        bool init(const std::string& filename);


        class Register {
        public:
            struct Field {
                const char* type = nullptr;
                const char* name = nullptr;
                void* storage = nullptr;
                const char* defval = nullptr;
                bool valmutable = false;
                Field() = default;
                Field(const char* ftype, const char* fname, void* fstorage, const char* fdefval,
                      bool fvalmutable)
                        : type(ftype),
                          name(fname),
                          storage(fstorage),
                          defval(fdefval),
                          valmutable(fvalmutable) {
                    // Initialize storage with default value
                    if (strcmp(ftype, "std::string") == 0) {
                        *static_cast<std::string*>(fstorage) = std::string(fdefval);
                    } else if (strcmp(ftype, "bool") == 0) {
                        *static_cast<bool*>(fstorage) = (strcmp(fdefval, "true") == 0 || strcmp(fdefval, "1") == 0);
                    } else if (strcmp(ftype, "int16_t") == 0) {
                        *static_cast<int16_t*>(fstorage) = static_cast<int16_t>(std::stoi(fdefval));
                    } else if (strcmp(ftype, "int32_t") == 0) {
                        *static_cast<int32_t*>(fstorage) = std::stoi(fdefval);
                    } else if (strcmp(ftype, "int64_t") == 0) {
                        *static_cast<int64_t*>(fstorage) = std::stoll(fdefval);
                    } else if (strcmp(ftype, "double") == 0) {
                        *static_cast<double*>(fstorage) = std::stod(fdefval);
                    }
                }

                void set_value(const std::string& str_value) const {
                    if (strcmp(type, "std::string") == 0) {
                        *static_cast<std::string*>(storage) = str_value;
                    } else if (strcmp(type, "bool") == 0) {
                        *static_cast<bool*>(storage) = (str_value == "true" || str_value == "1");
                    } else if (strcmp(type, "int16_t") == 0) {
                        *static_cast<int16_t*>(storage) = static_cast<int16_t>(std::stoi(str_value));
                    } else if (strcmp(type, "int32_t") == 0) {
                        *static_cast<int32_t*>(storage) = std::stoi(str_value);
                    } else if (strcmp(type, "int64_t") == 0) {
                        *static_cast<int64_t*>(storage) = std::stoll(str_value);
                    } else if (strcmp(type, "double") == 0) {
                        *static_cast<double*>(storage) = std::stod(str_value);
                    }
                }
            };

            static std::map<std::string, Field>* _s_field_map;

            Register(const char* ftype, const char* fname, void* fstorage, const char* fdefval,
                     bool fvalmutable) {
                if (_s_field_map == nullptr) {
                    _s_field_map = new std::map<std::string, Field>();
                }
                Field field(ftype, fname, fstorage, fdefval, fvalmutable);
                _s_field_map->insert(std::make_pair(std::string(fname), field));
            }
        };
    };
}
