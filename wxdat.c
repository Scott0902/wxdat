// 增加SSE2指令处理xor运算
// 单线程处理
// dat文件数：244
// 总体积：385.547 MB。
// 第一次运行
// 解密耗时：1.76 秒。
// 处理速度：219.061 MB/秒。
// 第二次运行（有系统缓存）
// 解密耗时：0.683 秒。
// 处理速度：564.491 MB/秒。

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <windows.h>
#include <time.h>

#define MAX_FILES 3000

long long GetAllFormatFiles(char* path, char* format, char** files, unsigned int* index)
{
    struct dirent* entry;
    DIR* dir = opendir(path);
    if (dir == NULL) {
        printf("输入路径无法访问：%s\n", path);
        return 0;
    }

    long long total_size = 0;
    while ((entry = readdir(dir)) != NULL) {
        char* filename = entry->d_name;
        char full_path[1024];
        sprintf(full_path, "%s\\%s", path, filename);
        struct stat st;
        stat(full_path, &st);
        if (S_ISREG(st.st_mode) && strlen(filename) >= strlen(format) && strcmp(filename + strlen(filename) - strlen(format), format) == 0) {
            char* full_path = (char*)malloc(strlen(path) + strlen(filename) + 2);
            sprintf(full_path, "%s\\%s", path, filename);
            files[*index] = full_path;
            (*index)++;
            total_size += st.st_size;
        } else if (S_ISDIR(st.st_mode) && strcmp(filename, ".") != 0 && strcmp(filename, "..") != 0) {
            char subdir[1024];
            sprintf(subdir, "%s\\%s", path, filename);
            total_size += GetAllFormatFiles(subdir, format, files, index);
        }
    }

    closedir(dir);
    return total_size;
}

unsigned char* read_file(char* filename, size_t* file_size)
{   
    FILE* file = fopen(filename, "rb");
    struct stat st;
    if (file == NULL)
    {printf("%s 打开文件错误\n", filename);
    }
  
    // Get file size
    if (stat(filename, &st) != 0)
    {printf("%s 获取文件大小失败\n", filename);
    }
    *file_size = st.st_size;
    // 一次性读取文件
    unsigned char* data = (unsigned char*)malloc(*file_size);
    if (data == NULL)
    {printf("%s 分配读入文件的内存失败\n", filename);
    }
    size_t bytes_read = fread(data, 1, *file_size, file);
    if (bytes_read != *file_size)
    {printf("%s 读取文件字节数不对\n", filename);
    }
    fclose(file);
    return data;
}

void SetOutputFilename(char *inputFile, char *outputPath, char *newExtension, char *outputFile) {
    char *inputFileName = strrchr(inputFile, '\\');
    if (inputFileName == NULL) {
        inputFileName = inputFile;
    } else {
        inputFileName++;
    }
    char *fileNameWithoutExtension = strtok(inputFileName, ".");
    snprintf(outputFile, 255, "%s\\%s%s", outputPath, fileNameWithoutExtension, newExtension);
}



void process_data(char* filename, char* output_path)
{
    size_t i = 0;
    size_t file_size = 0;
    uint8_t xor_key = 0;
    unsigned char* data = read_file(filename, &file_size);
    char* img_type;
    xor_key = data[0] ^ 0xff;
    if (xor_key == (data[1] ^ 0xd8)) {
        img_type = ".jpg";
    } else {
        xor_key = data[0] ^ 0x89;
        if (xor_key == (data[1] ^ 0x50)) {
            img_type = ".png";
        } else {
            xor_key = data[0] ^ 0x47;
            if (xor_key == (data[1] ^ 0x49)) {
                img_type = ".gif";
            } else {
                printf("判断不了该文件的图片类型：%s\n", filename);
                return;
            }
        }
    }

    __m128i xorKey = _mm_set1_epi8(xor_key);
    size_t multiple_16 = file_size>>4<<4;

    for (i = 0; i < multiple_16; i += 16) {
        __m128i *m128i_data = (__m128i *)&data[i];
        __m128i loadedData = _mm_loadu_si128(m128i_data);
        __m128i result = _mm_xor_si128(loadedData, xorKey);
        _mm_storeu_si128(m128i_data, result);
    }

    for (; i < file_size; ++i) {
        data[i] ^= xor_key;
    }


    char outputfile[1024];
    SetOutputFilename(filename, output_path, img_type, outputfile);
    FILE* file = fopen(outputfile, "wb");
    if (file == NULL) {
        printf("%s 创建输出文件失败\n", outputfile);
        return;
    }
    size_t written = fwrite(data, 1, file_size, file);
    if (written != file_size) {
        printf("%s写入文件失败\n", outputfile);
        return;
    }
    fclose(file);
    free(data);
    return;
}



int main(int argc, const char* argv[])
{
    UINT currentCodePage = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
    if (argc <= 1)
    {
        printf("wxdat.exe -- 解密微信电脑版的 dat 格式文件还原为图片格式。\n使用方法：wxdat.exe <dat文件路径> [输出路径]\n如果不指定“输出路径”，则默认输出到dat文件路径。\n\n");
        return 0;
    }
    char input_path[255], output_path[255];
    strncpy(input_path, argv[1], sizeof(input_path) - 1);
    input_path[sizeof(input_path) - 1] = '\0';

    if (argc == 2) {
        strncpy(output_path, argv[1], sizeof(output_path) - 1);
    } else {
        strncpy(output_path, argv[2], sizeof(output_path) - 1);
    }
    output_path[sizeof(output_path) - 1] = '\0';

    clock_t start_time = clock();    
    char* filelist[MAX_FILES];
    unsigned int filenumber = 0;
    long long total_size = GetAllFormatFiles(input_path, ".dat", filelist, &filenumber);

    unsigned int bar_size = 60,
    finished_bar = 0,
    i = 0, j = 0;
    for (i = 0; i < filenumber; i++) {
        printf("\r[");
        finished_bar=(unsigned int)((i+1)/(float)filenumber*bar_size);
        for (unsigned int j=0; j<finished_bar; j++) printf("#");
        for (unsigned int j=0; j<bar_size - finished_bar; j++) printf(".");
        printf("]");
        process_data(filelist[i], output_path);
        fflush(stdout);
    }

    double duration = (double)(clock() - start_time) / CLOCKS_PER_SEC;
    printf("\n解密耗时：%.3f 秒。\ndat文件数：%d\n总体积：%.3f MB。\n处理速度：%.3f MB/秒。\n", duration, filenumber, total_size/1048576.0, total_size/1048576.0/duration);
    for (i = 0; i < filenumber; i++) {
        free(filelist[i]);
    }
    SetConsoleOutputCP(currentCodePage);
    return 0;
}
