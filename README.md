## Сборка
```bash
mkdir build && cd build
cmake ..
make
```

## Запуск
``` bash
./build/test_kv_storage
```

## Асимптотика методов
Конструктор - O(N*log N)  
set() - O(log N)  
remove() - O(log N)  
get() - O(log N)  
getManySorted() - O(log N + M), где M = count  
removeOneExpiredEntry() - O(1)(амортизированно)  

## Число байт оверхэда
Для записи с TTL (ttl != 0)  
153 + 2 * key.size() + value.size() байт  
где records_: 24(ключ) + key.size() + 24 (значение) + value.size() + 1(optional) + 8(time_point) + 32(std::map)  
expiry_queue_: 8(time_point) + 24(ключ) + key.size() + 32(std::set)  

Для записи без TTL(ttl = 0)  
81 + key.size() + value.size() байт  
24 (ключ) + key.size() + 24 (значение) + value.size() + 1 (optional) + 32 (std::map)  
