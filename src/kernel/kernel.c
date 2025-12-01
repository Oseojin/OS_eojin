volatile char*  video_memory = (volatile char*)0xb8000;

void    print_string(const char* str, int offset)
{
    volatile char*  ptr = video_memory + offset * 2;
    int             i = 0;
    while (str[i] != 0)
    {
        *ptr = str[i];
        *(ptr + 1) = 0x0f;
        ptr += 2;
        i++;
    }
}

void    main()
{
    print_string("Hello from C Kernel!", 1);
}