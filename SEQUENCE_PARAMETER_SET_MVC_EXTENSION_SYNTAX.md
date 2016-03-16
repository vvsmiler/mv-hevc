# 개요 #
AVC는 MVC를 수행 하기 위한 모든 정보를 SPS에 담아 두었다.
이 내용은 HEVC를 이용한 MVC에도 필수적으로 필요한 내용이다.

# 세부 사항 #
먼저, AVC의 seq\_parameter\_set\_mvc\_extension()을 살펴 본다.
참고로, 이 NAL의 nal\_unit\_type은 0xF 이다. (NAL\_UNIT\_SUBSET\_SPS)

```
|seq_parameter_set_mvc_extension() {                                   |  C  | Descriptor |
|  num_views_minus1                                                    |  0  | ue(v)      |
|  for( i=0; i<=num_views_minus1; i++ )                                |     |            |
|    view_id[i]                                                        |  0  | ue(v)      |
|  for( i=0; i<=num_views_minus1; i++ ) {                              |     |            |
|    num_anchor_refs_l0[i]                                             |  0  | ue(v)      |
|    for( j=0; j<num_anchor_refs_l0[i]; j++ )                          |     |            |
|      anchor_ref_l0[i][j]                                             |  0  | ue(v)      |
|    num_anchor_refs_l1[i]                                             |  0  | ue(v)      |
|    for( j=0; j<num_anchor_refs_l1[i]; j++ )                          |     |            |
|      anchor_ref_l[i][j]                                              |  0  | ue(v)      |
|  }                                                                   |     |            |
|  for( i=0; i<=num_views_minus1; i++ ) {                              |     |            |
|    num_non_anchor_refs_l0[i]                                         |  0  | ue(v)      |
|    for( j=0; j<num_non_anchor_refs_l0[i]; j++ )                      |     |            |
|      non_anchor_ref_l0[i][j]                                         |  0  | ue(v)      |
|    num_non_anchor_refs_l1[i]                                         |  0  | ue(v)      |
|    for( j=0; j<num_non_anchor_refs_l1[i]; j++ )                      |     |            |
|      non_anchor_ref_l[i][j]                                          |  0  | ue(v)      |
|  }                                                                   |     |            |
|  num_level_values_signalled_minus1                                   |  0  | ue(v)      |
|  for( i=0; i<=num_level_values_signalled_minus1; i++ ) {             |     |            |
|    level_idc[i]                                                      |  0  | u(8)       |
|    num_applicable_ops_minus1[i]                                      |  0  | ue(v)      |
|    for( j=0; j<=num_applicable_ops_minus1[i]; j++ ) {                |     |            |
|      applicable_op_temporal_id[i][j]                                 |  0  | u(3)       |
|      applicable_op_num_target_views_minus1[i][j]                     |  0  | ue(v)      |
|      for( k=0; k<=applicable_op_num_target_views_minus1[i][j]; k++ ) |     |            |
|        applicable_op_target_view_id[i][j][k]                         |  0  | ue(v)      |
|      applicable_op_num_views_minus1[i][j]                            |  0  | ue(v)      |
|    }                                                                 |     |            |
|  }                                                                   |     |            |
|}                                                                     |     |            |
```

위의 내용 중 사용 할 만한 내용은 mvc 관련 설정들 뿐이다. level 관련 설정들은 HEVC에는 적용하기 곤란하다.
그리고, AVC의 SPS NAL 구조를 참고 하면 다음과 같다.

```
|seq_parameter_set_rbsp() {                                            |  C  | Descriptor |
|  seq_parameter_set_data()                                            |  0  |            |
|  rbsp_trailing_bits()                                                |  0  |            |
|}                                                                     |     |            |
```

```
|seq_parameter_set_data() {                                              |  C  | Descriptor |
|  profile_idc                                                           |  0  | u(8)       |
|  constraint_set0_flag                                                  |  0  | u(1)       |
|  constraint_set1_flag                                                  |  0  | u(1)       |
|  constraint_set2_flag                                                  |  0  | u(1)       |
|  constraint_set3_flag                                                  |  0  | u(1)       |
|  constraint_set4_flag                                                  |  0  | u(1)       |
|  constraint_set5_flag                                                  |  0  | u(1)       |
|  reserved_zero_2bits /* equal to 0 */                                  |  0  | u(2)       |
|  level_idc                                                             |  0  | u(8)       |
|  seq_parameter_set_id                                                  |  0  | ue(v)      |
|  if( profile_idc == 100 || profile_idc == 110 ||                       |     |            |
|      profile_idc == 122 || profile_idc == 244 || profile_idc ==  44 || |     |            |
|      profile_idc ==  83 || profile_idc ==  86 || profile_idc == 118 || |     |            |
|      profile_idc == 128 ) {                                            |     |            |
|    chroma_format_idc                                                   |  0  | ue(v)      |
|    if( chroma_format_idc == 3 )                                        |     |            |
|      separate_colour_plane_flag                                        |  0  | u(1)       |
|    bit_depth_luma_minus8                                               |  0  | ue(v)      |
|    bit_depth_chroma_minus8                                             |  0  | ue(v)      |
|    qpprime_y_zero_transform_bypass_flag                                |  0  | u(1)       |
|    seq_scaling_matrix_present_flag                                     |  0  | u(1)       |
|    if( seq_scaling_matrix_present_flag )                               |     |            |
|      for( i=0; i<( ( chroma_format_idc != 3 ) ? 8 : 12 ); i++ ) {      |     |            |
|        seq_scaling_list_present_flag[i]                                |  0  | u(1)       |
|        if( seq_scaling_list_present_flag[i] )                          |     |            |
|          if( i<6 )                                                     |     |            |
|            scaling_list( ScalingList4x4[i], 16,                        |  0  |            |
|                          UseDefaultScalingMatrix4x4Flag[i] )           |     |            |
|          else                                                          |     |            |
|            scaling_list( ScalingList8x8[i-6], 64,                      |  0  |            |
|                          UseDefaultScalingMatrix8x8Flag[i-6] )         |     |            |
|      }                                                                 |     |            |
|    }                                                                   |     |            |
|  log2_max_frame_num_minus4                                             |  0  | ue(v)      |
|  pic_order_cnt_type                                                    |  0  | ue(v)      |
|  if( pic_order_cnt_type == 0 )                                         |     |            |
|    log2_max_pic_order_cnt_lsb_minus4                                   |  0  | ue(v)      |
|  else if( pic_order_cnt_type == 1 ) {                                  |     |            |
|    delta_pic_order_always_zero_flag                                    |  0  | u(1)       |
|    offset_for_non_ref_pic                                              |  0  | se(v)      |
|    offset_for_top_to_bottom_field                                      |  0  | se(v)      |
|    num_ref_frames_in_pic_order_cnt_cycle                               |  0  | ue(v)      |
|    for( i=0; i<num_ref_frames_in_pic_order_cnt_cycle; i++ )            |     |            |
|      offset_for_ref_frame[i]                                           |  0  | se(v)      |
|  }                                                                     |     |            |
|  max_num_ref_frames                                                    |  0  | ue(v)      |
|  gaps_in_frame_num_value_allowed_flag                                  |  0  | u(1)       |
|  pic_width_in_mbs_minus1                                               |  0  | ue(v)      |
|  pic_height_in_map_units_minus1                                        |  0  | ue(v)      |
|  frame_mbs_only_flag                                                   |  0  | u(1)       |
|  if( !frame_mbs_only_flag )                                            |     |            |
|    mb_adaptive_frame_field_flag                                        |  0  | u(1)       |
|  direct_8x8_inference_flag                                             |  0  | u(1)       |
|  frame_cropping_flag                                                   |  0  | u(1)       |
|  if( frame_cropping_flag ) {                                           |     |            |
|    frame_crop_left_offset                                              |  0  | ue(v)      |
|    frame_crop_right_offset                                             |  0  | ue(v)      |
|    frame_crop_top_offset                                               |  0  | ue(v)      |
|    frame_crop_bottom_offset                                            |  0  | ue(v)      |
|  }                                                                     |     |            |
|  vui_parameters_present_flag                                           |  0  | u(1)       |
|  if( vui_parameters_present_flag )                                     |     |            |
|    vui_parameters()                                                    |     |            |
|}                                                                       |     |            |
```

```
|subset_seq_parameter_set_rbsp() {                                       |  C  | Descriptor |
|  seq_parameter_set_data()                                              |  0  |            |
|  if( profile_idc == 83 || profile_idc == 86 ) {                        |     |            |
|    seq_parameter_set_svc_extension() /* specified in Annex G */        |  0  |            |
|    svc_vui_parameters_present_flag                                     |  0  | u(1)       |
|    if( svc_vui_parameters_present_flag == 1 )                          |     |            |
|      svc_vui_parameters_extension() /* specified in Annex G */         |  0  |            |
|  } else if( profile_idc == 118 || profile_idc == 128 ) {               |     |            |
|    bit_equal_to_one /* equal to 1 */                                   |  0  | f(1)       |
|    seq_parameter_set_mvc_extension() /* specified in Annex H */        |  0  |            |
|    mvc_vui_parameters_present_flag                                     |  0  | u(1)       |
|    if( mvc_vui_parameters_present_flag == 1 )                          |     |            |
|      mvc_vui_parameters_extension() /* specified in Annex H */         |  0  |            |
|  }                                                                     |     |            |
|  additional_extension2_flag                                            |  0  | u(1)       |
|  if( additional_extension2_flag == 1 )                                 |     |            |
|    while( more_rbsp_data() )                                           |     |            |
|      additional_extension2_data_flag                                   |  0  | u(1)       |
|    rbsp_trailing_bits()                                                |  0  |            |
|}                                                                       |     |            |
```

HEVC의 SPS는 다음과 같다.
```
|seq_parameter_set_rbsp() {                                              | Descriptor |
|  profile_idc                                                           | u(8)       |
|  reserved_zero_8bits /* equal to 0 */                                  | u(8)       |
|  level_idc                                                             | u(8)       |
|  seq_parameter_set_id                                                  | ue(v)      |
|  pic_width_in_luma_samples                                             | u(16)      |
|  pic_height_in_luma_samples                                            | u(16)      |
|  bit_depth_luma_minus8                                                 | ue(v)      |
|  bit_depth_chroma_minus8                                               | ue(v)      |
|  bit_depth_luma_increment                                              | ue(v)      |
|  bit_depth_chroma_increment                                            | ue(v)      |
|  log2_max_frame_num_minus4                                             | ue(v)      |
|  pic_order_cnt_type                                                    | ue(v)      |
|  if( pic_order_cnt_type == 0 )                                         |            |
|    log2_max_pic_order_cnt_lsb_minus4                                   | ue(v)      |
|  else if( pic_order_cnt_type == 1 ) {                                  |            |
|    delta_pic_order_always_zero_flag                                    | u(1)       |
|    offset_for_non_ref_pic                                              | se(v)      |
|    num_ref_frames_in_pic_order_cnt_cycle                               | ue(v)      |
|    for( i=0; i<num_ref_frames_in_pic_order_cnt_cycle; i++ )            |            |
|      offset_for_ref_frame[i]                                           | se(v)      |
|  }                                                                     |            |
|  max_num_ref_frames                                                    | ue(v)      |
|  gaps_in_frame_num_value_allowed_flag                                  | u(1)       |
|  log2_min_coding_block_size_minus3                                     | ue(v)      |
|  log2_diff_max_min_coding_block_size                                   | ue(v)      |
|  log2_min_transform_block_size_minus2                                  | ue(v)      |
|  log2_diff_max_min_transform_block_size                                | ue(v)      |
|  max_transform_hierarchy_depth_inter                                   | ue(v)      |
|  max_transform_hierarchy_depth_intra                                   | ue(v)      |
|  interpolation_filter_flag                                             | u(1)       |
|  rbsp_trailing_bits()                                                  |            |
|}                                                                       |            |
```

HEVC에 쓸 SPS MVC EXTENSION을 정의 해 보면 다음과 같다.
```
|seq_parameter_set_mvc_extension() {                                     | Descriptor |
|  num_views_minus1                                                      | ue(v)      |
|  for( i=0; i<=num_views_minus1; i++ )                                  |            |
|    view_id[i]                                                          | ue(v)      |
|  for( i=0; i<=num_views_minus1; i++ ) {                                |            |
|    num_anchor_refs_l0[i]                                               | ue(v)      |
|    for( j=0; j<num_anchor_refs_l0[i]; j++ )                            |            |
|      anchor_ref_l0[i][j]                                               | ue(v)      |
|    num_anchor_refs_l1[i]                                               | ue(v)      |
|    for( j=0; j<num_anchor_refs_l1[i]; j++ )                            |            |
|      anchor_ref_l[i][j]                                                | ue(v)      |
|  }                                                                     |            |
|  for( i=0; i<=num_views_minus1; i++ ) {                                |            |
|    num_non_anchor_refs_l0[i]                                           | ue(v)      |
|    for( j=0; j<num_non_anchor_refs_l0[i]; j++ )                        |            |
|      non_anchor_ref_l0[i][j]                                           | ue(v)      |
|    num_non_anchor_refs_l1[i]                                           | ue(v)      |
|    for( j=0; j<num_non_anchor_refs_l1[i]; j++ )                        |            |
|      non_anchor_ref_l[i][j]                                            | ue(v)      |
|  }                                                                     |            |
|}                                                                       |            |
```

합쳐 보면 다음과 같겠다.
```
|seq_parameter_set_rbsp() {                                              | Descriptor |
|  profile_idc                                                           | u(8)       |
|  reserved_zero_8bits /* equal to 0 */                                  | u(8)       |
|  level_idc                                                             | u(8)       |
|  seq_parameter_set_id                                                  | ue(v)      |
|  pic_width_in_luma_samples                                             | u(16)      |
|  pic_height_in_luma_samples                                            | u(16)      |
|  bit_depth_luma_minus8                                                 | ue(v)      |
|  bit_depth_chroma_minus8                                               | ue(v)      |
|  bit_depth_luma_increment                                              | ue(v)      |
|  bit_depth_chroma_increment                                            | ue(v)      |
|  log2_max_frame_num_minus4                                             | ue(v)      |
|  pic_order_cnt_type                                                    | ue(v)      |
|  if( pic_order_cnt_type == 0 )                                         |            |
|    log2_max_pic_order_cnt_lsb_minus4                                   | ue(v)      |
|  else if( pic_order_cnt_type == 1 ) {                                  |            |
|    delta_pic_order_always_zero_flag                                    | u(1)       |
|    offset_for_non_ref_pic                                              | se(v)      |
|    num_ref_frames_in_pic_order_cnt_cycle                               | ue(v)      |
|    for( i=0; i<num_ref_frames_in_pic_order_cnt_cycle; i++ )            |            |
|      offset_for_ref_frame[i]                                           | se(v)      |
|  }                                                                     |            |
|  max_num_ref_frames                                                    | ue(v)      |
|  gaps_in_frame_num_value_allowed_flag                                  | u(1)       |
|  log2_min_coding_block_size_minus3                                     | ue(v)      |
|  log2_diff_max_min_coding_block_size                                   | ue(v)      |
|  log2_min_transform_block_size_minus2                                  | ue(v)      |
|  log2_diff_max_min_transform_block_size                                | ue(v)      |
|  max_transform_hierarchy_depth_inter                                   | ue(v)      |
|  max_transform_hierarchy_depth_intra                                   | ue(v)      |
|  interpolation_filter_flag                                             | u(1)       |
|  seq_parameter_set_mvc_extension()                                     |            |
|  rbsp_trailing_bits()                                                  |            |
|}                                                                       |            |
```