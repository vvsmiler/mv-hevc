# 개요 #

HEVC는 AVC의 문법 구조를 대부분 그대로 계승하고 있다.
따라서, NAL unit header의 문법 구조 역시 비슷한 개요를 가지고 있어 차이점만 분석하면 HEVC의 NAL unit header의 세부사항을 습득 할 수 있다.

이에 더해, AVC에추가된 MVC를 위한 고려 사항도 리뷰 하여 향후 구현에 참조 한다.


# 세부 사항 #

AVC의 NAL unit syntax
```
|nal_unit( NumBytesInNALunit ) {                                  |  C  | Descriptor |
|  forbidden_zero_bit                                             | All | f(1)       |
|  nal_ref_idc                                                    | All | u(2)       |
|  nal_unit_type                                                  | All | u(5)       |
|  NumBytesInRBSP = 0                                             |     |            |
|  nalUnitHeaderBytes = 1                                         |     |            |
|  if( nal_unit_type == 14 || nal_unit_type == 20 ) {             |     |            |
|    svc_extension_flag                                           | All | u(1)       |
|    if( svc_extension_flag )                                     |     |            |
|      nal_unit_header_svc_extension() // specified in Annex G    | All |            |
|    else                                                         |     |            |
|      nal_unit_header_mvc_extension() // specified in Annex H    | All |            |
|    nalUnitHeaderBytes += 3                                      |     |            |
|  }                                                              |     |            |
|  for( i=nalUnitHeaderBytes; i<NumBytesInNALunit; i++ ) {        |     |            |
|    if( i+2<NumBytesInNALunit && next_bits(24) == 0x000003) {    |     |            |
|      rbsp_byte[NumBytesInRBSP++]                                | All | b(8)       |
|      rbsp_byte[NumBytesInRBSP++]                                | All | b(8)       |
|      i += 2                                                     |     |            |
|      emulation_prevention_three_byte // equal to 0x03           |     | f(8)       |
|    } else                                                       |     |            |
|      rbsp_byte[NumBytesInRBSP++]                                |     | b(8)       |
|  }                                                              |     |            |
|}                                                                |     |            |
```

AVC의 NAL unit header MVC extension syntax
```
|nal_unit_header_mvc_extension() {                                |  C  | Descriptor |
|  non_idr_flag                                                   | All | u(1)       |
|  priority_id                                                    | All | u(6)       |
|  view_id                                                        | All | u(10)      |
|  temporal_id                                                    | All | u(3)       |
|  anchor_pic_flag                                                | All | u(1)       |
|  inter_view_flag                                                | All | u(1)       |
|  reserved_one_bit                                               | All | u(1)       |
|}                                                                |     |            |
```

HEVC의 NAL unit syntax
```
|nal_unit( NumBytesInNALunit ) {                                  | Descriptor |
|  forbidden_zero_bit                                             | f(1)       |
|  nal_ref_idc                                                    | u(2)       |
|  nal_unit_type                                                  | u(5)       |
|  NumBytesInRBSP = 0                                             |            |
|  nalUnitHeaderBytes = 1                                         |            |
|  if( nal_unit_type == 1 || nal_unit_type == 5 ) {               |            |
|    temporal_flag                                                | u(3)       |
|    output_flag                                                  | u(1)       |
|    reserved_zero_4bits                                          | u(4)       |
|    nalUnitHeaderBytes += 1                                      |            |
|  }                                                              |            |
|  for( i=nalUnitHeaderBytes; i<NumBytesInNALunit; i++ ) {        |            |
|    if( i+2<NumBytesInNALunit && next_bits(24) == 0x000003) {    |            |
|      rbsp_byte[NumBytesInRBSP++]                                | b(8)       |
|      rbsp_byte[NumBytesInRBSP++]                                | b(8)       |
|      i += 2                                                     |            |
|      emulation_prevention_three_byte // equal to 0x03           | f(8)       |
|    } else                                                       |            |
|      rbsp_byte[NumBytesInRBSP++]                                | b(8)       |
|  }                                                              |            |
|}                                                                |            |
```

위의 HEVC의 NAL unit syntax에 MVC extension syntax를 적용 해 보면 다음과 같다.
```
|nal_unit( NumBytesInNALunit ) {                                  | Descriptor |
|  forbidden_zero_bit                                             | f(1)       |
|  nal_ref_idc                                                    | u(2)       |
|  nal_unit_type                                                  | u(5)       |
|  NumBytesInRBSP = 0                                             |            |
|  nalUnitHeaderBytes = 1                                         |            |
|  if( nal_unit_type == 1 || nal_unit_type == 5 ) {               |            |
|    temporal_flag                                                | u(3)       |
|    output_flag                                                  | u(1)       |
|    reserved_zero_4bits                                          | u(4)       |
|    nalUnitHeaderBytes += 1                                      |            |
|  } else if( nal_unit_type == 14 || nal_unit_type == 20 ) {      |            |
|    nal_unit_header_mvc_extension()                              |            |
|    nalUnitHeaderBytes += 3                                      |            |
|  }                                                              |            |
|  for( i=nalUnitHeaderBytes; i<NumBytesInNALunit; i++ ) {        |            |
|    if( i+2<NumBytesInNALunit && next_bits(24) == 0x000003) {    |            |
|      rbsp_byte[NumBytesInRBSP++]                                | b(8)       |
|      rbsp_byte[NumBytesInRBSP++]                                | b(8)       |
|      i += 2                                                     |            |
|      emulation_prevention_three_byte // equal to 0x03           | f(8)       |
|    } else                                                       |            |
|      rbsp_byte[NumBytesInRBSP++]                                | b(8)       |
|  }                                                              |            |
|}                                                                |            |
```