add_library(lte SHARED
  src/liblte_common.cc
  src/liblte_interface.cc
  src/liblte_rlc.cc
  src/liblte_pdcp.cc
  src/liblte_rrc.cc
  src/liblte_mme.cc
  src/liblte_security.cc
)
include_directories(hdr)
