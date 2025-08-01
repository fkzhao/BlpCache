#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <fstream>

#define DECLARE_FIELD(FIELD_TYPE, FIELD_NAME) extern FIELD_TYPE FIELD_NAME

#define DECLARE_Bool(name) DECLARE_FIELD(bool, name)
#define DECLARE_Int16(name) DECLARE_FIELD(int16_t, name)
#define DECLARE_Int32(name) DECLARE_FIELD(int32_t, name)
#define DECLARE_Int64(name) DECLARE_FIELD(int64_t, name)
#define DECLARE_Double(name) DECLARE_FIELD(double, name)
#define DECLARE_String(name) DECLARE_FIELD(std::string, name)
#define DECLARE_Bools(name) DECLARE_FIELD(std::vector<bool>, name)
#define DECLARE_Int16s(name) DECLARE_FIELD(std::vector<int16_t>, name)
#define DECLARE_Int32s(name) DECLARE_FIELD(std::vector<int32_t>, name)
#define DECLARE_Int64s(name) DECLARE_FIELD(std::vector<int64_t>, name)
#define DECLARE_Doubles(name) DECLARE_FIELD(std::vector<double>, name)
#define DECLARE_Strings(name) DECLARE_FIELD(std::vector<std::string>, name)
#define DECLARE_mBool(name) DECLARE_FIELD(bool, name)
#define DECLARE_mInt16(name) DECLARE_FIELD(int16_t, name)
#define DECLARE_mInt32(name) DECLARE_FIELD(int32_t, name)
#define DECLARE_mInt64(name) DECLARE_FIELD(int64_t, name)
#define DECLARE_mDouble(name) DECLARE_FIELD(double, name)
#define DECLARE_mString(name) DECLARE_FIELD(std::string, name)

#define DEFINE_FIELD(FIELD_TYPE, FIELD_NAME, FIELD_DEFAULT, VALMUTABLE)                      \
    FIELD_TYPE FIELD_NAME;                                                                   \
    static Register reg_##FIELD_NAME(#FIELD_TYPE, #FIELD_NAME, &(FIELD_NAME), FIELD_DEFAULT, \
                                     VALMUTABLE);

#define DEFINE_VALIDATOR(FIELD_NAME, VALIDATOR)              \
    static auto validator_##FIELD_NAME = VALIDATOR;          \
    static RegisterConfValidator reg_validator_##FIELD_NAME( \
            #FIELD_NAME, []() -> bool { return validator_##FIELD_NAME(FIELD_NAME); });

#define DEFINE_Int16(name, defaultstr) DEFINE_FIELD(int16_t, name, defaultstr, false)
#define DEFINE_Bools(name, defaultstr) DEFINE_FIELD(std::vector<bool>, name, defaultstr, false)
#define DEFINE_Doubles(name, defaultstr) DEFINE_FIELD(std::vector<double>, name, defaultstr, false)
#define DEFINE_Int16s(name, defaultstr) DEFINE_FIELD(std::vector<int16_t>, name, defaultstr, false)
#define DEFINE_Int32s(name, defaultstr) DEFINE_FIELD(std::vector<int32_t>, name, defaultstr, false)
#define DEFINE_Int64s(name, defaultstr) DEFINE_FIELD(std::vector<int64_t>, name, defaultstr, false)
#define DEFINE_Bool(name, defaultstr) DEFINE_FIELD(bool, name, defaultstr, false)
#define DEFINE_Double(name, defaultstr) DEFINE_FIELD(double, name, defaultstr, false)
#define DEFINE_Int32(name, defaultstr) DEFINE_FIELD(int32_t, name, defaultstr, false)
#define DEFINE_Int64(name, defaultstr) DEFINE_FIELD(int64_t, name, defaultstr, false)
#define DEFINE_String(name, defaultstr) DEFINE_FIELD(std::string, name, defaultstr, false)
#define DEFINE_Strings(name, defaultstr) \
    DEFINE_FIELD(std::vector<std::string>, name, defaultstr, false)
#define DEFINE_mBool(name, defaultstr) DEFINE_FIELD(bool, name, defaultstr, true)
#define DEFINE_mInt16(name, defaultstr) DEFINE_FIELD(int16_t, name, defaultstr, true)
#define DEFINE_mInt32(name, defaultstr) DEFINE_FIELD(int32_t, name, defaultstr, true)
#define DEFINE_mInt64(name, defaultstr) DEFINE_FIELD(int64_t, name, defaultstr, true)
#define DEFINE_mDouble(name, defaultstr) DEFINE_FIELD(double, name, defaultstr, true)
#define DEFINE_mString(name, defaultstr) DEFINE_FIELD(std::string, name, defaultstr, true)
#define DEFINE_Validator(name, validator) DEFINE_VALIDATOR(name, validator)

namespace blp {
    class Status;

    namespace config {
        DECLARE_String(custom_config_dir);

        DECLARE_String(persistent_dir);

        DECLARE_Int32(port);

        DECLARE_String(host);

        DECLARE_Int32(replcate_port);


        class Register {
        public:
            struct Field {
                const char *type = nullptr;
                const char *name = nullptr;
                void *storage = nullptr;
                const char *defval = nullptr;
                bool valmutable = false;

                Field(const char *ftype, const char *fname, void *fstorage, const char *fdefval,
                      bool fvalmutable)
                    : type(ftype),
                      name(fname),
                      storage(fstorage),
                      defval(fdefval),
                      valmutable(fvalmutable) {
                }
            };

        public:
            static std::map<std::string, Field> *_s_field_map;

        public:
            Register(const char *ftype, const char *fname, void *fstorage, const char *fdefval,
                     bool fvalmutable) {
                if (_s_field_map == nullptr) {
                    _s_field_map = new std::map<std::string, Field>();
                }
                Field field(ftype, fname, fstorage, fdefval, fvalmutable);
                _s_field_map->insert(std::make_pair(std::string(fname), field));
            }
        };

        // RegisterConfValidator class is used to store validator function of registered config fields in
        // Register::_s_field_map.
        // If any validator return false when BE bootstart, the bootstart will be terminated.
        // If validator return false when use http API to update some config, the config will not
        // be modified and the API will return failure.
        class RegisterConfValidator {
        public:
            // Validator for each config name.
            static std::map<std::string, std::function<bool()> > *_s_field_validator;

        public:
            RegisterConfValidator(const char *fname, const std::function<bool()> &validator) {
                if (_s_field_validator == nullptr) {
                    _s_field_validator = new std::map<std::string, std::function<bool()> >();
                }
                // register validator to _s_field_validator
                _s_field_validator->insert(std::make_pair(std::string(fname), validator));
            }
        };

        // full configurations.
        extern std::map<std::string, std::string> *full_conf_map;

        extern std::mutex custom_conf_lock;

        // Init the config from `conf_file`.
        // If fill_conf_map is true, the updated config will also update the `full_conf_map`.
        // If must_exist is true and `conf_file` does not exist, this function will return false.
        // If set_to_default is true, the config value will be set to default value if not found in `conf_file`.
        bool init(const char *conf_file, bool fill_conf_map = false, bool must_exist = true,
                  bool set_to_default = true);

        Status set_config(const std::string &field, const std::string &value, bool need_persist = false,
                          bool force = false);

        Status persist_config(const std::string &field, const std::string &value);

        std::mutex *get_mutable_string_config_lock();

        std::vector<std::vector<std::string> > get_config_info();

        Status set_fuzzy_configs();

        void update_config(const std::string &field, const std::string &value);
    }
}
