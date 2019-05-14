/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <copying.hpp>
#include <cudf.h>
#include <cuda_runtime.h>
#include "utilities/error_utils.hpp"

namespace cudf
{

/*
 * Initializes and returns gdf_column of the same type as the input.
 */
gdf_column empty_like(gdf_column const& input)
{
  CUDF_EXPECTS(input.size == 0 || input.data != 0, "Null input data");
  gdf_column output;

  CUDF_EXPECTS(GDF_SUCCESS == 
               gdf_column_view_augmented(&output, nullptr, nullptr, 0,
                                         input.dtype, 0, input.dtype_info),
               "Invalid column parameters");

  return output;
}

/*
 * Allocates a new column of the same size and type as the input.
 * Does not copy data.
 */
gdf_column allocate_like(gdf_column const& input, cudaStream_t stream)
{
  gdf_column output = empty_like(input);
  
  output.size = input.size;
  if (input.size > 0) {
    const auto byte_width = (input.dtype == GDF_STRING)
                          ? sizeof(std::pair<const char *, size_t>)
                          : gdf_dtype_size(input.dtype);
    RMM_TRY(RMM_ALLOC(&output.data, input.size * byte_width, stream));
    if (input.valid != nullptr) {
      size_t valid_size = gdf_valid_allocation_size(input.size);
      RMM_TRY(RMM_ALLOC(&output.valid, valid_size, stream));
    }
  }

  return output;
}

/*
 * Creates a new column that is a copy of input
 */
gdf_column copy(gdf_column const& input, cudaStream_t stream)
{
  CUDF_EXPECTS(input.size == 0 || input.data != 0, "Null input data");

  gdf_column output = allocate_like(input, stream);

  if (input.size > 0) {
    const auto byte_width = (input.dtype == GDF_STRING)
                          ? sizeof(std::pair<const char *, size_t>)
                          : gdf_dtype_size(input.dtype);
    CUDA_TRY(cudaMemcpyAsync(output.data, input.data, input.size * byte_width,
                             cudaMemcpyDefault, stream));
    if (input.valid != nullptr) {
      size_t valid_size = gdf_valid_allocation_size(input.size);
      CUDA_TRY(cudaMemcpyAsync(output.valid, input.valid, valid_size,
                               cudaMemcpyDefault, stream));
    }
  }

  return output;
}

} // namespace cudf