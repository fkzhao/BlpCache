
# BlpCache

**BlpCache** is a lightweight distributed caching system that supports **standard Redis protocol**, **read-write separation**, and **High Availability (HA) mode**. It is designed to provide a simple, efficient, and highly available caching solution for high-performance, high-concurrency scenarios.

BlpCache is compatible with existing Redis clients and ecosystems, allowing seamless integration without modifying your application code.

---

## Features
- ⚡ Lightweight and easy to deploy
- 📡 Compatible with standard Redis protocol
- 🔄 Read-Write Separation architecture
- 🛡️ High Availability (HA) with automatic failover
- 🚀 Optimized for high-concurrency read/write operations
- 🧩 Modular and plugin-friendly architecture

---

## Getting Started

### 1. Clone the Repository
```bash
git clone https://github.com/fkzhao/BlpCache.git
cd BlpCache
````

### 2. Build the Project

```bash
mkdir build && cd build
cmake ..
make -j4
```

### 3. Start the BlpCache Server

```bash
./blpCache --config ../config/blp.conf
```

### 4. Connect using a Redis Client

```bash
# Example using redis-cli
redis-cli -h 127.0.0.1 -p 6379 set key1 value1
redis-cli -h 127.0.0.1 -p 6379 get key1
```

---

## Dependencies

* CMake >= 3.15
* GCC >= 9.0 / Clang >= 10.0
* LevelDB >= 1.22
* BRPC >= 1.5.0
* gflags / glog
* protobuf >= 3.15
* pthread (POSIX Thread)

### Install Dependencies (Debian/Ubuntu Example)

```bash
sudo apt update
sudo apt install -y build-essential cmake libgflags-dev libprotobuf-dev protobuf-compiler libgoogle-glog-dev libsnappy-dev
# For BRPC and LevelDB installation, refer to docs/install.md
```

---

## Configuration

All configuration options are managed through `config/blpcache.yaml`. Key configuration sections include:

* **network:** Service listening ports and Redis protocol settings
* **storage:** Backend storage configuration (e.g., LevelDB path)
* **replication:** Master-Slave replication settings
* **ha\_mode:** High Availability parameters

For a detailed configuration guide, refer to [Configuration Guide](docs/configuration.md).

---

## Architecture (Optional)

```
+-------------+       +-----------------+       +-------------+
|   Client    | <---> |   BlpCache Node | <---> |   Backend   |
| (Redis CLI) |       | (Master/Replica)|       | (LevelDB)   |
+-------------+       +-----------------+       +-------------+
                          |       |
                          |  +---------+
                          +->|  Master  |
                          |  +---------+
                          |  +---------+
                          +->|  Replica |
                             +---------+
```
![image](docs/arch.png)


---

## Testing

```bash
# Run unit tests
make test

# Run integration tests (requires Docker)
cd tests/integration
docker-compose up --build
```

---

## Contributing

We welcome contributions via Pull Requests and Issues! To contribute:

1. Fork this repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -m 'Add your feature'`)
4. Push to your branch (`git push origin feature/your-feature`)
5. Open a Pull Request

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for more details.

---

## License

BlpCache is licensed under the MIT License. See [LICENSE](LICENSE) for full license text.

## Documents
[中文版](docs/README_CN.md)