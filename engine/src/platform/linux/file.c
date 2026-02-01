#include "platform/file.h"

#ifdef PLATFORM_LINUX_FLAG

    #include "core/logger.h"
    #include "debug/assert.h"

    #include <stdio.h>
    #include <unistd.h>
    #include <string.h>
    #include <sys/stat.h>

    bool platform_file_open(const char* path, platform_file_mode_flag mode, platform_file** out_file)
    {
        ASSERT(path != nullptr, "String pointer must be non-null.");
        ASSERT(mode != 0, "Mode mask must be greater than zero.");

        char mode_str[4] = {};
        u32 index = 0;
        *out_file = nullptr;

        // Возможные варианты:
        // - r(b) чтение                                     / r(b)+ чтение и запись
        // - w(b) запись с перезапесью                       / w(b)+ чтение и запись с перезаписью
        // - a(b) создание, открытие с записью в конец файла / a(b)+ добавление

        // Первый символ последовательности режима файла.
        if(mode & PLATFORM_FILE_MODE_READ)
        {
            mode_str[index++] = 'r';
        }
        else if(mode & PLATFORM_FILE_MODE_WRITE)
        {
            mode_str[index++] = 'w';
        }
        else if(mode & PLATFORM_FILE_MODE_APPEND)
        {
            mode_str[index++] = 'a';
        }
        else
        {
            LOG_ERROR("Failed to open file '%s': Invalid file mode %d.", path, mode);
            return false;
        }

        // Второй символ последовательности режима файла.
        if(mode & PLATFORM_FILE_MODE_BINARY && index == 1)
        {
            mode_str[index++] = 'b';
        }

        // Третий символ последовательности режима файла.
        if(mode & PLATFORM_FILE_MODE_WRITE && index >= 1)
        {
            mode_str[index++] = '+';
        }

        // Конечный символ последовательности режима файла (нулевой терминатор).
        mode_str[index] = '\0';

        FILE* file = fopen(path, mode_str);
        if(file == nullptr)
        {
            LOG_ERROR("Failed to open file '%s': Cannot open.", path);
            return false;
        }

        // NOTE: Вместо выделения памяти, указываем, что это указатель на контекст.
        *out_file = (platform_file*)file;
        return true;
    }

    void platform_file_close(platform_file* file)
    {
        ASSERT(file != nullptr, "Pointer to file must be non-null.");
        fclose((FILE*)file);
    }

    bool platform_file_sync(platform_file* file)
    {
        ASSERT(file != nullptr, "Pointer to file must be non-null.");
        return fflush((FILE*)file) != EOF;
    }

    bool platform_file_exists(const char* path)
    {
        ASSERT(path != nullptr, "Pointer to file must be non-null.");
        return access(path, F_OK) == 0;
    }

    bool platform_file_size(const platform_file* file, u64* size)
    {
        ASSERT(file != nullptr, "Pointer to file must be non-null.");
        ASSERT(size != nullptr, "Pointer to get file size must be non-null.");

        // Получение файлового дескриптора из структуры FILE.
        int fd = fileno((FILE*)file);
        struct stat file_info;

        // Получение информации о файле.
        if(fstat(fd, &file_info) != 0)
        {
            *size = 0;
            return false;
        }

        *size = (u64)file_info.st_size;
        return true;
    }

    bool platform_file_read_line(const platform_file* file, u64 buffer_size, char* buffer, u64* out_length)
    {
        ASSERT(file != nullptr, "Pointer to file must be non-null.");
        ASSERT(buffer != nullptr, "Pointer to buffer must be non-null.");
        ASSERT(buffer_size > 0, "Buffer size must be greater than zero.");

        if(fgets(buffer, (int)buffer_size, (FILE*)file) == nullptr)
        {
            // Конец файла или ошибка чтения.
            if(out_length != nullptr) *out_length = 0;
            return false;
        }

        u64 length = strlen(buffer);
        if(length > 0)
        {
            // Удаление символа возврата коретки для Windows файлов.
            if(buffer[length - 1] == '\r') --length;

            // Удаление символа переноса на новую строку.
            if(buffer[length - 1] == '\n') --length;

            // Вставка нулевого символа в конец.
            buffer[length] = '\0';
        }

        if(out_length != nullptr)
        {
            *out_length = length;
        }

        return true;
    }

    bool platform_file_write_line(platform_file* file, const char* buffer)
    {
        ASSERT(file != nullptr, "Pointer to file must be non-null.");
        ASSERT(buffer != nullptr, "Pointer to buffer must be non-null.");

        // Запись новой строки и следом запись символа новой строки.
        return fputs(buffer, (FILE*)file) != EOF && fputc('\n', (FILE*)file) != EOF;
    }

    bool platform_file_read(platform_file* file, u64 data_size, void* data, u64* out_read_size)
    {
        ASSERT(file != nullptr, "Pointer to file must be non-null.");
        ASSERT(data != nullptr, "Pointer to data must be non-null.");
        ASSERT(data_size > 0, "Data size must be greater than zero.");

        size_t read_size = fread(data, 1, data_size, (FILE*)file);
        if(out_read_size != nullptr)
        {
            *out_read_size = (u64)read_size;
        }

        return read_size == data_size;
    }

    bool platform_file_write(platform_file* file, u64 data_size, const void* data)
    {
        ASSERT(file != nullptr, "Pointer to file must be non-null.");
        ASSERT(data != nullptr, "Pointer to data must be non-null.");
        ASSERT(data_size > 0, "Data size must be greater than zero.");

        return fwrite(data, 1, data_size, (FILE*)file) == data_size;
    }

#endif
