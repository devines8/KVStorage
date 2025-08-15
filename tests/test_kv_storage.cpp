#include "kv_storage.h"
#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

// TestClock для управления временем в тестах
class TestClock {
public:
  using time_point = system_clock::time_point;
  using duration = system_clock::duration;

  static time_point now() noexcept { return current_time; }
  static void advance(duration d) { current_time += d; }
  static void set(time_point tp) { current_time = tp; }

private:
  static time_point current_time;
};

TestClock::time_point TestClock::current_time = TestClock::time_point{};

class KVStorageTest : public ::testing::Test {
protected:
  void SetUp() override { TestClock::set(TestClock::time_point{}); }
};

// 1. Тесты конструктора и базовых операций
TEST_F(KVStorageTest, ConstructorInitializesEntries) {
  vector<tuple<string, string, uint32_t>> entries = {{"k1", "v1", 0},
                                                     {"k2", "v2", 10}};

  KVStorage<TestClock> storage(entries);
  EXPECT_EQ(storage.get("k1"), "v1");
  EXPECT_EQ(storage.get("k2"), "v2");
}

TEST_F(KVStorageTest, SetAndGet) {
  KVStorage<TestClock> storage({});
  storage.set("key", "value");
  EXPECT_EQ(storage.get("key"), "value");
}

// 2. Тесты TTL и времени
TEST_F(KVStorageTest, TTLExpiration) {
  KVStorage<TestClock> storage({});
  storage.set("temp", "value", 5); // 5 секунд TTL

  TestClock::advance(4s);
  EXPECT_EQ(storage.get("temp"), "value"); // Ещё жив

  TestClock::advance(2s);
  EXPECT_FALSE(storage.get("temp").has_value()); // Должен исчезнуть
}

TEST_F(KVStorageTest, PermanentEntry) {
  KVStorage<TestClock> storage({});
  storage.set("perm", "value");

  TestClock::advance(365 * 24h);           // 365 дней
  EXPECT_EQ(storage.get("perm"), "value"); // Должен остаться
}

// 3. Тесты удаления
TEST_F(KVStorageTest, RemoveExisting) {
  KVStorage<TestClock> storage({});
  storage.set("key", "value");

  EXPECT_TRUE(storage.remove("key"));
  EXPECT_FALSE(storage.get("key").has_value());
}

TEST_F(KVStorageTest, RemoveNonExisting) {
  KVStorage<TestClock> storage({});
  EXPECT_FALSE(storage.remove("nonexistent"));
}

// 4. Тесты getManySorted
TEST_F(KVStorageTest, GetManySortedBasic) {
  KVStorage<TestClock> storage({});
  storage.set("b", "val2");
  storage.set("a", "val1");
  storage.set("d", "val4");
  storage.set("c", "val3");

  auto result = storage.getManySorted("b", 2);
  ASSERT_EQ(result.size(), 2);
  EXPECT_EQ(result[0].first, "c");
  EXPECT_EQ(result[1].first, "d");
}

TEST_F(KVStorageTest, GetManySortedWithExpired) {
  KVStorage<TestClock> storage({});
  storage.set("a", "val1", 5);
  storage.set("b", "val2");
  storage.set("c", "val3", 10);

  TestClock::advance(6s);
  auto result = storage.getManySorted("a", 3);
  ASSERT_EQ(result.size(), 2); // "a" должен исчезнуть
  EXPECT_EQ(result[0].first, "b");
  EXPECT_EQ(result[1].first, "c");
}

// 5. Тесты removeOneExpiredEntry
TEST_F(KVStorageTest, RemoveOneExpired) {
  KVStorage<TestClock> storage({});
  storage.set("k1", "v1", 5);
  storage.set("k2", "v2", 10);

  TestClock::advance(6s);
  auto expired = storage.removeOneExpiredEntry();
  ASSERT_TRUE(expired.has_value());
  EXPECT_EQ(expired->first, "k1");
  EXPECT_FALSE(storage.get("k1").has_value());
  EXPECT_EQ(storage.get("k2"), "v2"); // Должен остаться
}

TEST_F(KVStorageTest, RemoveOneExpiredWhenNone) {
  KVStorage<TestClock> storage({});
  storage.set("k1", "v1");

  auto expired = storage.removeOneExpiredEntry();
  EXPECT_FALSE(expired.has_value());
}

// 6. Граничные случаи
TEST_F(KVStorageTest, EmptyStorage) {
  KVStorage<TestClock> storage({});
  EXPECT_FALSE(storage.get("any").has_value());
  EXPECT_FALSE(storage.remove("any"));
  EXPECT_TRUE(storage.getManySorted("a", 5).empty());
}

TEST_F(KVStorageTest, DuplicateKeys) {
  KVStorage<TestClock> storage({});
  storage.set("key", "v1");
  storage.set("key", "v2");

  EXPECT_EQ(storage.get("key"), "v2");
}

// 7. Тесты с большими данными
TEST_F(KVStorageTest, LargeDataSet) {
  KVStorage<TestClock> storage({});
  const int N = 1000;

  for (int i = 0; i < N; ++i) {
    storage.set("key" + to_string(i), "value" + to_string(i));
  }

  EXPECT_EQ(storage.get("key500"), "value500");
  EXPECT_TRUE(storage.remove("key123"));
  EXPECT_FALSE(storage.get("key123").has_value());
}

// 8. Тесты многопоточной работы
TEST_F(KVStorageTest, ConcurrentReadWrite) {
  KVStorage<TestClock> storage({});
  const int num_threads = 10;
  const int num_operations = 1000;

  vector<thread> threads;

  // Запускаем потоки для записи
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&storage, i]() {
      for (int j = 0; j < num_operations; ++j) {
        string key = "key_" + to_string(i) + "_" + to_string(j);
        string value = "value_" + to_string(i) + "_" + to_string(j);
        storage.set(key, value);
      }
    });
  }

  // Запускаем потоки для чтения
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&storage, i]() {
      for (int j = 0; j < num_operations; ++j) {
        string key = "key_" + to_string(i) + "_" + to_string(j);
        storage.get(key);
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Проверяем, что все данные записались корректно
  for (int i = 0; i < num_threads; ++i) {
    for (int j = 0; j < num_operations; ++j) {
      string key = "key_" + to_string(i) + "_" + to_string(j);
      string expected_value = "value_" + to_string(i) + "_" + to_string(j);
      auto value = storage.get(key);
      ASSERT_TRUE(value.has_value());
      EXPECT_EQ(*value, expected_value);
    }
  }
}

TEST_F(KVStorageTest, ConcurrentRemove) {
  KVStorage<TestClock> storage({});
  const int num_keys = 1000;

  // Инициализируем данные
  for (int i = 0; i < num_keys; ++i) {
    storage.set("key_" + to_string(i), "value_" + to_string(i));
  }

  vector<thread> threads;

  // Запускаем потоки для удаления
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&storage, i, num_keys]() {
      for (int j = i; j < num_keys; j += 10) {
        storage.remove("key_" + to_string(j));
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Проверяем, что все ключи были удалены
  for (int i = 0; i < num_keys; ++i) {
    EXPECT_FALSE(storage.get("key_" + to_string(i)).has_value());
  }
}

TEST_F(KVStorageTest, ConcurrentTTLExpiration) {
  KVStorage<TestClock> storage({});
  const int num_keys = 1000;

  vector<thread> threads;

  // Запускаем потоки для установки значений с TTL
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&storage, i]() {
      for (int j = 0; j < 100; ++j) {
        string key = "key_" + to_string(i) + "_" + to_string(j);
        storage.set(key, "value", 5); // 5 секунд TTL
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Перемещаем время вперед и проверяем истечение TTL
  TestClock::advance(6s);

  // Проверяем, что все ключи истекли
  for (int i = 0; i < 10; ++i) {
    for (int j = 0; j < 100; ++j) {
      string key = "key_" + to_string(i) + "_" + to_string(j);
      EXPECT_FALSE(storage.get(key).has_value());
    }
  }
}

TEST_F(KVStorageTest, ConcurrentRemoveOneExpired) {
  KVStorage<TestClock> storage({});
  const int num_keys = 1000;

  // Инициализируем данные с разным TTL
  for (int i = 0; i < num_keys; ++i) {
    storage.set("key_" + to_string(i), "value_" + to_string(i), i % 10 + 1);
  }

  TestClock::advance(5s); // Часть ключей уже истекла

  vector<thread> threads;
  atomic<int> expired_count{0};

  // Запускаем потоки для удаления истекших ключей
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&storage, &expired_count]() {
      for (int j = 0; j < 100; ++j) {
        if (storage.removeOneExpiredEntry().has_value()) {
          expired_count++;
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Проверяем, что примерно половина ключей истекла
  EXPECT_GT(expired_count, 400);
  EXPECT_LT(expired_count, 600);
}

TEST_F(KVStorageTest, MixedOperations) {
  KVStorage<TestClock> storage({});
  const int num_operations = 10000;
  const int num_threads = 10;

  vector<thread> threads;

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&storage, i, num_operations]() {
      for (int j = 0; j < num_operations; ++j) {
        string key = "key_" + to_string(i) + "_" + to_string(j);

        // Чередуем операции
        switch (j % 4) {
        case 0:
          storage.set(key, "value");
          break;
        case 1:
          storage.get(key);
          break;
        case 2:
          storage.remove(key);
          break;
        case 3:
          storage.set(key, "value", 10);
          break;
        }
      }
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  // Проверяем целостность хранилища
  for (int i = 0; i < num_threads; ++i) {
    for (int j = 0; j < num_operations; ++j) {
      string key = "key_" + to_string(i) + "_" + to_string(j);
      // Проверяем только ключи, которые должны существовать
      if (j % 4 == 0 || j % 4 == 3) {
        auto value = storage.get(key);
        EXPECT_TRUE(value.has_value() || !storage.get(key).has_value());
      }
    }
  }
}
