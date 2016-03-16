# 목표 #
HEVC에 Multi view video coding 기능을 추가 한다.

# 개요 #
최근 표준화를 진행 중인 HEVC에 H.264에 적용 된 Multi view video coding 기능을 Poring 한다.

# 기존 Video Coding Flow와 다른 점 #
  * Encoder
  1. NAL Header에 MVC 관련 추가 변수.
  1. SPS에 MVC 관련 추가 변수.
  1. Slice Header의 Reference Picture Order Modify 기능에 MVC 고려.
  1. 여러 채널의 Bitstream을 하나의 Bitstream으로 Assembly
  * Decoder
  1. 같은 POC를 가지고, View Order로 정렬된 View Component들을 하나의 Operation Unit으로 취급.
  1. SPS에 정의된 View Order라고 가정 하고 Decoding 시작함.
  1. View Component 만큼의 Reference Picture List를 유지 해야 함.