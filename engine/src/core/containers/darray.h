/*
    @file darray.h
    @brief Интерфейс динамического массива.
    @author Дмитрий Скляр.
    @version 1.0
    @date 14-09-2025

    @license Лицензия Apache, версия 2.0 («Лицензия»);
          Вы не имеете права использовать этот файл без соблюдения условий Лицензии.
          Копию Лицензии можно получить по адресу http://www.apache.org/licenses/LICENSE-2.0
          Если иное не предусмотрено действующим законодательством или не согласовано в письменной форме,
          программное обеспечение, распространяемое по Лицензии, распространяется на условиях «КАК ЕСТЬ»,
          БЕЗ КАКИХ-ЛИБО ГАРАНТИЙ ИЛИ УСЛОВИЙ, явных или подразумеваемых. См. Лицензию для получения
          информации о конкретных языках, регулирующих разрешения и ограничения по Лицензии.

    @note Предоставляет:
            - Автоматическое управление памятью с реаллокацией
            - Добавление/удаление элементов в начало, конец и середину
            - Работу с любыми типами данных через type-agnostic интерфейс
            - Безопасный доступ к элементам с проверкой границ (в debug-режиме)
            - Эффективное использование памяти с настраиваемым множителем емкости

    @note Особенности реализации:
            - Данные хранятся в одном непрерывном блоке памяти с заголовком
            - Заголовок содержит метаинформацию и находится перед данными массива
            - Для работы с массивом используются макросы (darray_*) для type safety
*/

#pragma once

#include <core/defines.h>

// @brief Заголовок динамического массива, хранящий метаданные.
typedef struct dynamic_array_header {
    // @brief Текущая емкость массива.
    u64 capacity;
    // @brief Текущее количество элементов.
    u64 length;
    // @brief Размер одного элемента в байтах.
    u64 stride;
    // @brief Гибкий массив для данных.
    u8 internal_data[];
} dynamic_array_header;

// @brief Стандартная начальная емкость массива.
#define DARRAY_DEFAULT_CAPACITY      1

// @brief Стандартный множитель для увеличения емкости.
#define DARRAY_DEFAULT_RESIZE_FACTOR 2

/*
    @brief Создает новый динамический массив.
    @param stride Размер одного элемента в байтах.
    @param capacity Начальная емкость массива.
    @return Указатель на массив или nullptr при ошибке выделения памяти.
*/
API void* dynamic_array_create(u64 stride, u64 capacity);

/*
    @brief Уничтожает динамический массив и освобождает память.
    @param array Указатель на массив.
*/
API void dynamic_array_destroy(void* array);

/*
    @brief Возвращает размер элемента массива в байтах.
    @param array Указатель на массив.
    @return Размер элемента.
*/
API u64 dynamic_array_stride(void* array);

/*
    @brief Возвращает текущее количество элементов в массиве.
    @param array Указатель на массив.
    @return Количество элементов.
*/
API u64 dynamic_array_length(void* array);

/*
    @brief Возвращает текущую емкость массива.
    @param array Указатель на массив.
    @return Емкость массива.
*/
API u64 dynamic_array_capacity(void* array);

/*
    @brief Добавляет элемент в конец массива.
    @param array Указатель на указатель массива (может измениться при реаллокации).
    @param data Указатель на данные для добавления.
*/
API void dynamic_array_push(void** array, const void* data);

/*
    @brief Удаляет последний элемент массива.
    @param array Указатель на массив.
    @param out_data Указатель для копирования удаляемого элемента (может быть nullptr).
*/
API void dynamic_array_pop(void* array, void* out_data);

/*
    @brief Вставляет элемент в указанную позицию.
    @param array Указатель на указатель массива (может измениться при реаллокации).
    @param index Позиция для вставки.
    @param data Указатель на данные для вставки.
*/
API void dynamic_array_insert(void** array, u64 index, const void* data);

/*
    @brief Удаляет элемент из указанной позиции.
    @param array Указатель на массив.
    @param index Позиция для удаления.
    @param out_data Указатель для копирования удаляемого элемента (может быть nullptr).
*/
API void dynamic_array_remove(void* array, u64 index, void* out_data);

/*
    @brief Очищает массив (устанавливает длину в 0, но сохраняет емкость).
    @param array Указатель на массив.
    @param zero_memory Если true, обнуляет всю память массива.
*/
API void dynamic_array_clear(void* array, bool zero_memory);

/*
    @brief Устанавливает новую длину массива.
    @param array Указатель массив.
    @param length Новая длина массива.
*/
API void dynamic_array_set_length(void* array, u64 length);

/*
    @brief Создает динамический массив для указанного типа.
    @param type Тип элементов массива.
    @return Указатель на созданный массив или nullptr.
*/
#define darray_create(type) (type*)dynamic_array_create(sizeof(type), DARRAY_DEFAULT_CAPACITY)

/*
    @brief Создает динамический массив для указанного типа и емкости.
    @param type Тип элементов массива.
    @param capacity Начальная емкость массива.
    @return Указатель на созданный массив или nullptr.
*/
#define darray_create_custom(type, capacity) (type*)dynamic_array_create(sizeof(type), capacity)

/*
    @brief Уничтожает динамический массив и освобождает память.
    @param array Указатель на массив.
*/
#define darray_destroy(array) dynamic_array_destroy((void*)array)

/*
    @brief Возвращает размер элемента массива в байтах.
    @param array Указатель на массив.
    @return Размер элемента.
*/
#define darray_stride(array) dynamic_array_stride((void*)array)

/*
    @brief Возвращает текущее количество элементов в массиве.
    @param array Указатель на массив.
    @return Количество элементов.
*/
#define darray_length(array) dynamic_array_length((void*)array)

/*
    @brief Возвращает текущую емкость массива.
    @param array Указатель на массив.
    @return Емкость массива.
*/
#define darray_capacity(array) dynamic_array_capacity((void*)array)

/*
    @brief Добавляет элемент в конец массива.
    @param array Указатель на массив (может измениться при реаллокации).
    @param value Значение для добавления.
*/
#define darray_push(array, value)                      \
    do {                                               \
        typeof(value) __temp = value;                  \
        dynamic_array_push((void**)&(array), &__temp); \
    } while(false)

/*
    @brief Удаляет последний элемент массива.
    @param array Указатель на массив.
    @param out_value Указатель для сохранения удаляемого значения (может быть nullptr).
*/
#define darray_pop(array, out_value) dynamic_array_pop((void*)array, out_value)

/*
    @brief Вставляет элемент в указанную позицию.
    @param array Указатель на массив (может измениться при реаллокации).
    @param index Позиция для вставки.
    @param value Значение для вставки.
*/
#define darray_insert(array, index, value)                      \
    do {                                                        \
        typeof(value) __temp = value;                           \
        dynamic_array_insert((void**)&(array), index, &__temp); \
    } while(false)

/*
    @brief Удаляет элемент из указанной позиции.
    @param array Указатель на массив.
    @param index Позиция для удаления.
    @param out_value Указатель для сохранения удаляемого значения (может быть nullptr).
*/
#define darray_remove(array, index, out_value) dynamic_array_remove((void*)array, index, out_value)

/*
    @brief Сбрасывает массив (устанавливает длину в 0 без обнуления памяти).
    @note Быстрее чем darray_clear(), но не обеспечивает безопасность данных.
    @note Используется когда производительность критична и данные не содержат конфиденциальной информации.
    @param array Указатель на массив.
*/
#define darray_reset(array) dynamic_array_clear(array, false)

/*
    @brief Очищает массив (устанавливает длину в 0 с полным обнулением памяти).
    @note Медленнее чем darray_reset(), но обеспечивает безопасность данных.
    @note Рекомендуется для конфиденциальных данных или когда требуется гарантированное обнуление.
    @param array Указатель на массив.
*/
#define darray_clear(array) dynamic_array_clear(array, true)

/*
    @brief Устанавливает новую длину массива.
    @param array Указатель на массив.
    @param length Новая длина массива.
*/
#define darray_set_length(array, length) dynamic_array_set_length(array, length)
