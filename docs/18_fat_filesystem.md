# FAT16 파일 시스템 구조

## 1. 개요
FAT(File Allocation Table)는 가장 널리 쓰였던 단순한 구조의 파일 시스템입니다. 
FAT12(플로피), FAT16(초기 하드디스크), FAT32(대용량)로 나뉘며, 우리는 구현 난이도와 호환성이 좋은 **FAT16**을 목표로 합니다.

## 2. 디스크 레이아웃
FAT 파일 시스템은 디스크를 다음 순서대로 나눕니다.

| 영역 이름 | 설명 |
| :--- | :--- |
| **Boot Sector (BPB)** | 파일 시스템 정보 (섹터 크기, 클러스터 크기 등) |
| **Reserved Sectors** | 사용하지 않음 (부트 섹터 포함) |
| **FAT 1** | 클러스터 체인 테이블 (원본) |
| **FAT 2** | 클러스터 체인 테이블 (백업) |
| **Root Directory** | 루트 폴더의 파일/디렉토리 목록 (고정 크기) |
| **Data Region** | 실제 파일 데이터가 저장되는 클러스터 영역 |

## 3. 주요 구조체

### 3.1. BPB (BIOS Parameter Block)
부트 섹터(`LBA 0` 또는 파티션 시작점)의 앞부분에 위치합니다.

```c
typedef struct {
    uint8_t  jump_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;    // 보통 512
    uint8_t  sectors_per_cluster; // 예: 1, 2, 4...
    uint16_t reserved_sectors;    // 보통 1 (부트섹터)
    uint8_t  fat_count;           // 보통 2
    uint16_t root_dir_entries;    // 루트 디렉토리 항목 수 (예: 512)
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;         // FAT 영역 크기 (섹터 수)
    // ... (이하 확장 필드)
} __attribute__((packed)) bpb_t;
```

### 3.2. Directory Entry (32 bytes)
파일이나 디렉토리 하나를 설명하는 정보입니다.

```c
typedef struct {
    uint8_t  name[8];       // 파일명 (8글자, 공백 패딩)
    uint8_t  ext[3];        // 확장자 (3글자)
    uint8_t  attributes;    // 속성 (읽기전용, 숨김, 디렉토리 등)
    uint8_t  reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high; // FAT32용 (FAT16은 0)
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;  // 시작 클러스터 번호
    uint32_t file_size;          // 파일 크기 (바이트)
} __attribute__((packed)) dir_entry_t;
```

## 4. 파일 읽기 과정
1. **BPB 읽기:** 데이터 영역의 시작 위치를 계산합니다.
   - `RootDirStart = ReservedSectors + (FatCount * FatSize)`
   - `DataStart = RootDirStart + (RootDirEntries * 32 / BytesPerSector)`
2. **Root Directory 탐색:** 파일 이름(`name`+`ext`)과 일치하는 엔트리를 찾습니다.
3. **Cluster Chain 추적:**
   - 엔트리에서 `first_cluster`를 얻습니다.
   - 해당 클러스터의 데이터를 읽습니다. (LBA = DataStart + (Cluster - 2) * SectorsPerCluster)
   - 파일이 클러스터보다 크다면, **FAT 테이블**에서 다음 클러스터 번호를 조회합니다.
   - `0xFFFF` (End of Chain)가 나올 때까지 반복합니다.
