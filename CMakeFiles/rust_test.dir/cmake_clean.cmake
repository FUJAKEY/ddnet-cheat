file(REMOVE_RECURSE
  "librust_test.a"
  "librust_test.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang C CXX)
  include(CMakeFiles/rust_test.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
