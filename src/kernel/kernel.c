volatile char*  video_memory = (volatile char*)0xb8000;

void main()
{
    // 화면 왼쪽 상단에 'X' 출력
    *video_memory = 'X';
    // 속성 바이트 (흰색 글자 / 검은 배경)
    *(video_memory + 1) = 0x0f;
}