#include <stdio.h>
#include "encode.h"
#include "types.h"
#include <string.h>
#include "common.h"
/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (an int)
    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    // Read the height (an int)
    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    // Return image capacity
    return width * height * 3;
}

/*
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    // Src Image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    // Do Error handling
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

        return e_failure;
    }

    // Secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    // Do Error handling
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

        return e_failure;
    }

    // Stego Image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    // Do Error handling
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

        return e_failure;
    }

    // No failure return e_success
    return e_success;
}


//checking for encoding or decoding
OperationType check_operation_type(char *argv[])
{
    //encoding
    if (!strcmp(argv[1], "-e"))
        return e_encode;
    //decoding
    else if (!strcmp(argv[1], "-d"))
        return e_decode;
    else
        return e_unsupported;
}


//validating all the files and storing 
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if (strstr(argv[2], ".bmp"))
    {
        encInfo->src_image_fname = argv[2];

        if (strstr(argv[3], ".txt")||strstr(argv[3], ".c")||strstr(argv[3], ".sh"))
        {
            encInfo->secret_fname = argv[3];
            //printf("%s\n",strchr(encInfo->secret_fname, '.'));

            if (argv[4])
            {
                encInfo->stego_image_fname = argv[4];
            }
            else
            {
                encInfo->stego_image_fname = "Ouptut.bmp";
            }
        }
        else
        {
            return e_failure;
        }
        return e_success;
    }
    else
    {
        return e_failure;
    }
}

Status do_encoding(EncodeInfo *encInfo)
{
    if (open_files(encInfo) == e_success)
    {
        printf("Files opened succesfully\n");
        get_image_size_for_bmp(encInfo->fptr_src_image);
    }
    else
    {
        printf("Failed to open files\n");
        return e_failure;
    }
    if (check_capacity(encInfo) == e_success)
    {
        printf("File size validated!!\n");
    }
    else
    {
        printf("Fail to validate fail size\n");
        return e_failure;
    }
    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success)
    {
        printf("Congo!!..your file header has been copied\n");
    }
    else
    {
        printf("Header copy failed");
        return e_failure;
    }
    if (encode_magic_string(MAGIC_STRING, encInfo) == e_success)
    {
        printf("Magic string encoded successfully!!\n");
    }
    else
    {
        printf("Sorry..Failed to encode magic string\n");
        return e_failure;
    }
    if(encode_extn_file_size(strlen(strchr(encInfo->secret_fname,'.')),encInfo)==e_success)
    {
        printf("Extention length encoded\n");
    }
    else
    {
        printf("Extention length encoding failed\n");
        return e_failure;
    }
    if (encode_secret_file_extn(strchr(encInfo->secret_fname, '.'), encInfo) == e_success)
    {
        printf("Encoded sceret file extn !!\n");
    }
    else
    {
        printf("Sorry..Failed to encode secert file extn\n");
        return e_failure;
    }
    if (encode_secret_file_size(get_file_size(encInfo->fptr_secret), encInfo) == e_success)
    {
        printf("File size encoded successfully\n");
    }
    else
    {
        printf("Sorry..Failed to encode secret file extn\n");
        return e_failure;
    }
    if(encode_secret_file_data(encInfo)== e_success)
    {
        printf("Secret file data encoded\n");
    }
    else
    {
        printf("failed to encode secret file data");
        return e_failure;
    }
    if(copy_remaining_img_data(encInfo->fptr_src_image,encInfo->fptr_stego_image)==e_success)
    {
        return e_success;
    }
    else
    {
        return e_failure;
    }
}

uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    int size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    return size;
}

Status check_capacity(EncodeInfo *encInfo)
{
    if (get_file_size(encInfo->fptr_src_image) > (54 + strlen(MAGIC_STRING) + 4 + strlen(strchr(encInfo->secret_fname, '.')) + 4 + get_file_size(encInfo->fptr_secret) * 8))
    {
        return e_success;
    }
    else
        return e_failure;
}

Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char arr[54];
    fread(arr, 54, 1, fptr_src_image);
    fwrite(arr, 54, 1, fptr_dest_image);
    if (ftell(fptr_dest_image) == 54)
    {
        return e_success;
    }
    else
    {
        return e_failure;
    }
}

Status encode_data_to_image(char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char arr[8];
    for (int i = 0; i < size; i++)
    {
        fread(arr, 8, 1, fptr_src_image);
        encode_byte_to_lsb(data[i], arr);
        fwrite(arr, 8, 1, fptr_stego_image);
    }
    return e_success;
}
Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        image_buffer[i] = image_buffer[i] & 0xfe | data >> i & 0x01;
        //image_buffer[i] = 0xFF;
    }
    return e_success;
}

Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{

    if (encode_data_to_image((char *)magic_string, strlen(magic_string), encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success)
    {
        return e_success;
    }
}

Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    if (encode_data_to_image((char *)file_extn, strlen(file_extn), encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_success)
    {
        return e_success;
    }
}

Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    if(encode_size_to_lsb(file_size,encInfo)==e_success)
    {
        return e_success;
    }
  
}

Status encode_secret_file_data(EncodeInfo *encInfo)
{
    int size=get_file_size(encInfo->fptr_secret);
    char buff[8];
    rewind(encInfo->fptr_secret);
    for (int i = 0; i < size; i++)
    {
        fread(buff,8,1,encInfo->fptr_src_image);
        encode_byte_to_lsb(fgetc(encInfo->fptr_secret),buff);
        fwrite(buff,8,1,encInfo->fptr_stego_image);
    }
    return e_success;
}

Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char ch;
    printf("Copying remaining data\n");

    while(fread(&ch,1,1,fptr_src))
    {
      fwrite(&ch,1,1,fptr_dest);
    }
    return e_success;
}

Status encode_extn_file_size(long size,EncodeInfo *encInfo)
{
    if(encode_size_to_lsb(size,encInfo)==e_success)
    {
        return e_success;
    }
    else
    {
        return e_failure;
    }
}

Status encode_size_to_lsb(long size,EncodeInfo *encInfo)
{
    char arr[32];
    fread(arr,32,1,encInfo->fptr_src_image);
    for (int i = 0; i < 32; i++)
    {
        arr[i] = arr[i] & 0xFFFFFFFE | (size >> i) & 0x00000001;
    }
    fwrite(arr,32,1,encInfo->fptr_stego_image);
    return e_success;
}