#pragma once

#include <core/defines.h>

// @brief Контекст файла.
typedef struct platform_file platform_file;

// @brief Флаги режима открытия файла (комбинируются через побитовое ИЛИ).
typedef enum platform_file_mode_flag {
    // @brief Режим чтения файла.
    PLATFORM_FILE_MODE_READ             = 0x01,
    // @brief Режим записи файла (перезаписывает существующий).
    PLATFORM_FILE_MODE_WRITE            = 0x02,
    // @brief Бинарный режим работы с файлом.
    PLATFORM_FILE_MODE_BINARY           = 0x04,
    // @brief Режим добавления данных в конец файла.
    PLATFORM_FILE_MODE_APPEND           = 0x08,
    // @brief Режим чтения текстового файла построчно.
    PLATFORM_FILE_MODE_READ_TEXT_LINES  = PLATFORM_FILE_MODE_READ,
    // @brief Режим записи текстового файла построчно.
    PLATFORM_FILE_MODE_WRITE_TEXT_LINES = PLATFORM_FILE_MODE_WRITE,
    // @brief Режим чтения бинарного файла.
    PLATFORM_FILE_MODE_READ_BINARY      = PLATFORM_FILE_MODE_READ  | PLATFORM_FILE_MODE_BINARY,
    // @brief Режим записи бинарного файла.
    PLATFORM_FILE_MODE_WRITE_BINARY     = PLATFORM_FILE_MODE_WRITE | PLATFORM_FILE_MODE_BINARY
} platform_file_mode_flag;

/*
    @brief Открывает файл по указанному пути.
    @param path Путь к файлу.
    @param mode Режим открытия файла (комбинация флагов).
    @param out_file Указатель для сохранения созданного контекста файла.
    @return true файл успешно открыт, false ошибка открытия.
*/
API bool platform_file_open(const char* path, platform_file_mode_flag mode, platform_file** out_file);

/*
    @brief Закрывает файл и освобождает связанные с ним ресурсы.
    @warning После вызова этой функции указатель на файл становится недействительным!
    @param file Контекст файла для закрытия.
*/
API void platform_file_close(platform_file* file);

/*
    @brief Синхронизирует файл с диском, гарантируя запись всех данных.
    @param file Контекст файла для записи на диск.
    @return true файла успешно синхронизирован, false ошибка синхронизации.
*/
API bool platform_file_sync(platform_file* file);

/*
    @brief Проверяет существование файла по указанному пути.
    @param path Путь файла существование которого необходимо проверить.
    @return true файл существует, false файл не найден.
*/
API bool platform_file_exists(const char* path);

/*
    @brief Возвращает размер указанного файла в байтах.
    @param file Контекст файла размер которого необходимо получить.
    @param size Указатель для сохранения размера файла в байтах.
    @return true операция успешно завершена, false ошибка операции.
*/
API bool platform_file_size(const platform_file* file, u64* size);

/*
    @brief Читает следующую строку из указанного текстового файла.
    @param file Контекст файла строку из которого необходимо прочитать.
    @param buffer_size Размер буфера для чтения (включая нулевой терминатор).
    @param buffer Буфер для сохранения прочитанной из файла строки.
    @param out_length Указатель для сохранения количества прочитанных символов (без учета терминатора, может быть nullptr).
    @param true строка успешно прочитана, false ошибка чтения или достигнут конец файла.
*/
API bool platform_file_read_line(const platform_file* file, u64 buffer_size, char* buffer, u64* out_length);

/*
    @brief Записывает строку текста в файл.
    @param file Контекст файла.
    @param buffer Строка для записи (должна заканчиваться нулевым символом).
    @return true строка успешно записана, false ошибка записи.
*/
API bool platform_file_write_line(platform_file* file, const char* buffer);

/*
    @brief Читает байты данных из файла.
    @param file Контекст файла.
    @param data_size Количество байт для чтения (U64_MAX - прочитать весь файл).
    @param data Буфер для сохранения прочитанных данных.
    @param out_read_size Указатель для сохранения количества фактически прочитанных байт (может быть nullptr).
    @param true данные успешно прочитаны, false ошибка чтения.
*/
API bool platform_file_read(platform_file* file, u64 data_size, void* data, u64* out_read_size);

/*
    @brief Записывает байты данных в файл.
    @note  Рекомендуется перед закрытием файла выполнить синхронизацию файла с диском.
    @param file Контекст файла.
    @param data_size Количество байт для записи.
    @param data Буфер с данными для записи.
    @param true данные успешно записаны, false ошибка записи.
*/
API bool platform_file_write(platform_file* file, u64 data_size, const void* data);
