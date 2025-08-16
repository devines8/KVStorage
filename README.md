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
