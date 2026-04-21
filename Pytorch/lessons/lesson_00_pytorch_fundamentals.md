# Lesson 00: PyTorch Fundamentals

## Goal

This lesson introduces the core building block of PyTorch: the tensor.

By the end, you should understand how to:

- Import PyTorch and check your version.
- Create tensors of different shapes and dimensions.
- Inspect tensor shape, datatype, and device.
- Perform basic tensor operations.
- Use matrix multiplication correctly.
- Reshape, stack, squeeze, unsqueeze, and permute tensors.
- Index into tensors.
- Convert between PyTorch tensors and NumPy arrays.
- Make random operations more reproducible.
- Move tensors between CPU, CUDA GPUs, Apple Silicon GPUs, and back.

## What Is PyTorch?

PyTorch is an open source machine learning and deep learning framework.

It lets you write Python code to:

- Represent data numerically.
- Process and manipulate that data.
- Build machine learning and deep learning models.
- Use hardware acceleration such as GPUs.

PyTorch is widely used in research and industry by companies and labs such as Meta, Tesla, Microsoft, and OpenAI.

## Why Use PyTorch?

PyTorch is popular because it gives you a Python-friendly way to write machine learning code while handling many performance details for you.

The main benefits are:

- It works naturally with Python.
- It is widely used in deep learning research.
- It supports GPU acceleration.
- It has a large ecosystem and active community.
- It makes tensor operations and neural network building blocks easier to work with.

## Importing PyTorch

Start every PyTorch notebook or script by importing `torch`.

```python
import torch

torch.__version__
```

In Google Colab, PyTorch is usually already installed.

## Tensors

A tensor is a numerical container for data.

Machine learning models cannot directly understand raw images, text, audio, or tables. Those inputs must be represented as numbers. Tensors are the structure PyTorch uses to store and manipulate those numbers.

For example, an image can be represented as a tensor with shape:

```text
[3, 224, 224]
```

This could mean:

- `3` color channels: red, green, blue
- `224` pixels high
- `224` pixels wide

## Scalars, Vectors, Matrices, and Tensors

### Scalar

A scalar is a single number. In PyTorch, it is a zero-dimensional tensor.

```python
scalar = torch.tensor(7)

print(scalar)
print(scalar.ndim)
print(scalar.item())
```

Output idea:

```text
tensor(7)
0
7
```

Use `.item()` to extract a Python number from a one-element tensor.

### Vector

A vector is a one-dimensional tensor.

```python
vector = torch.tensor([7, 7])

print(vector)
print(vector.ndim)
print(vector.shape)
```

Output idea:

```text
tensor([7, 7])
1
torch.Size([2])
```

The vector has one dimension because it has one outer level of square brackets.

### Matrix

A matrix is a two-dimensional tensor.

```python
MATRIX = torch.tensor([[7, 8],
                       [9, 10]])

print(MATRIX)
print(MATRIX.ndim)
print(MATRIX.shape)
```

Output idea:

```text
tensor([[ 7,  8],
        [ 9, 10]])
2
torch.Size([2, 2])
```

### Higher-Dimensional Tensor

A tensor can have any number of dimensions.

```python
TENSOR = torch.tensor([[[1, 2, 3],
                        [3, 6, 9],
                        [2, 4, 5]]])

print(TENSOR)
print(TENSOR.ndim)
print(TENSOR.shape)
```

Output idea:

```text
3
torch.Size([1, 3, 3])
```

The shape moves from outer dimensions to inner dimensions.

## Naming Convention

Common math notation often uses:

- Lowercase letters for scalars and vectors: `a`, `y`
- Uppercase letters for matrices and tensors: `X`, `W`

In PyTorch, everything is usually a `torch.Tensor`, but the shape tells you whether you are working with a scalar, vector, matrix, or higher-dimensional tensor.

| Name | Meaning | Dimensions | Example Shape |
| --- | --- | ---: | --- |
| Scalar | Single number | 0 | `torch.Size([])` |
| Vector | One-dimensional list of numbers | 1 | `torch.Size([3])` |
| Matrix | Two-dimensional table of numbers | 2 | `torch.Size([3, 2])` |
| Tensor | N-dimensional array of numbers | Any | `torch.Size([1, 3, 3])` |

## Random Tensors

Machine learning models usually start with random numbers, then update those numbers while learning from data.

Use `torch.rand()` to create random tensors.

```python
random_tensor = torch.rand(size=(3, 4))

print(random_tensor)
print(random_tensor.dtype)
```

Example image-shaped tensor:

```python
random_image_size_tensor = torch.rand(size=(224, 224, 3))

print(random_image_size_tensor.shape)
print(random_image_size_tensor.ndim)
```

## Zeros and Ones

Use `torch.zeros()` and `torch.ones()` when you need tensors filled with only zeros or ones.

```python
zeros = torch.zeros(size=(3, 4))
ones = torch.ones(size=(3, 4))

print(zeros)
print(ones)
```

These are commonly used for masking, initialization, and placeholder tensors.

## Ranges

Use `torch.arange()` to create a tensor containing a range of numbers.

```python
zero_to_ten = torch.arange(start=0, end=10, step=1)

print(zero_to_ten)
```

Output:

```text
tensor([0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
```

`torch.range()` is deprecated. Prefer `torch.arange()`.

## Tensors Like Other Tensors

Use `torch.zeros_like()` or `torch.ones_like()` to create tensors with the same shape as another tensor.

```python
zero_to_ten = torch.arange(0, 10)
ten_zeros = torch.zeros_like(input=zero_to_ten)

print(ten_zeros)
```

## Tensor Datatypes

Common tensor datatypes include:

- `torch.float32` or `torch.float`: common default floating-point type
- `torch.float16` or `torch.half`: lower precision floating-point type
- `torch.float64` or `torch.double`: higher precision floating-point type
- `torch.int8`, `torch.int16`, `torch.int32`, `torch.int64`: integer types

Example:

```python
float_32_tensor = torch.tensor([3.0, 6.0, 9.0],
                               dtype=None,
                               device=None,
                               requires_grad=False)

print(float_32_tensor.shape)
print(float_32_tensor.dtype)
print(float_32_tensor.device)
```

Create a float16 tensor:

```python
float_16_tensor = torch.tensor([3.0, 6.0, 9.0],
                               dtype=torch.float16)

print(float_16_tensor.dtype)
```

Three common sources of PyTorch errors are:

- Shape mismatch
- Datatype mismatch
- Device mismatch

When debugging, ask:

```text
What shape is it?
What datatype is it?
Where is it stored?
```

## Getting Tensor Information

The three most important tensor attributes are:

- `.shape`
- `.dtype`
- `.device`

```python
some_tensor = torch.rand(3, 4)

print(some_tensor)
print(f"Shape of tensor: {some_tensor.shape}")
print(f"Datatype of tensor: {some_tensor.dtype}")
print(f"Device tensor is stored on: {some_tensor.device}")
```

## Basic Tensor Operations

PyTorch supports normal arithmetic operations.

```python
tensor = torch.tensor([1, 2, 3])

print(tensor + 10)
print(tensor * 10)
print(tensor - 10)
```

These operations do not change the original tensor unless you reassign the result.

```python
tensor = tensor + 10
```

You can also use PyTorch functions:

```python
torch.multiply(tensor, 10)
torch.add(tensor, 10)
```

But operator syntax is usually more common:

```python
tensor * 10
tensor + 10
```

## Element-Wise Multiplication

Element-wise multiplication multiplies matching positions.

```python
tensor = torch.tensor([1, 2, 3])

print(tensor * tensor)
```

Output:

```text
tensor([1, 4, 9])
```

This calculation is:

```text
[1*1, 2*2, 3*3]
```

## Matrix Multiplication

Matrix multiplication is one of the most important operations in deep learning.

Use:

```python
torch.matmul(a, b)
```

or:

```python
a @ b
```

Example:

```python
tensor = torch.tensor([1, 2, 3])

torch.matmul(tensor, tensor)
```

Output:

```text
tensor(14)
```

This calculation is:

```text
1*1 + 2*2 + 3*3 = 14
```

## Matrix Multiplication Shape Rules

There are two main rules:

1. The inner dimensions must match.
2. The output shape is made from the outer dimensions.

Examples:

```text
(3, 2) @ (3, 2) -> error
(2, 3) @ (3, 2) -> works, output shape (2, 2)
(3, 2) @ (2, 3) -> works, output shape (3, 3)
```

Example error:

```python
tensor_A = torch.tensor([[1, 2],
                         [3, 4],
                         [5, 6]], dtype=torch.float32)

tensor_B = torch.tensor([[7, 10],
                         [8, 11],
                         [9, 12]], dtype=torch.float32)

torch.matmul(tensor_A, tensor_B)
```

This errors because both tensors have shape `(3, 2)`, so the inner dimensions do not match.

Fix it by transposing `tensor_B`.

```python
output = torch.matmul(tensor_A, tensor_B.T)

print(output)
print(output.shape)
```

Now the multiplication is:

```text
(3, 2) @ (2, 3) -> (3, 3)
```

## Linear Layers

Neural networks use matrix multiplication constantly.

`torch.nn.Linear()` performs a linear transformation:

```text
y = xA^T + b
```

Where:

- `x` is the input
- `A` is the weights matrix
- `b` is the bias
- `y` is the output

Example:

```python
torch.manual_seed(42)

linear = torch.nn.Linear(in_features=2,
                         out_features=6)

x = tensor_A
output = linear(x)

print(f"Input shape: {x.shape}")
print(output)
print(f"Output shape: {output.shape}")
```

`in_features` must match the last dimension of the input.

## Aggregation

Aggregation means reducing many values into fewer values.

Common aggregation methods:

- `.min()`
- `.max()`
- `.mean()`
- `.sum()`

Example:

```python
x = torch.arange(0, 100, 10)

print(f"Minimum: {x.min()}")
print(f"Maximum: {x.max()}")
print(f"Mean: {x.type(torch.float32).mean()}")
print(f"Sum: {x.sum()}")
```

Some operations, such as `.mean()`, require floating-point tensors.

You can also use function syntax:

```python
torch.max(x)
torch.min(x)
torch.mean(x.type(torch.float32))
torch.sum(x)
```

## Positional Min and Max

Use `argmax()` and `argmin()` to find the index position of the maximum or minimum value.

```python
tensor = torch.arange(10, 100, 10)

print(tensor.argmax())
print(tensor.argmin())
```

This is useful when you care about where a value occurs, not just what the value is.

## Changing Tensor Datatypes

Use `.type()` to convert a tensor to another datatype.

```python
tensor = torch.arange(10., 100., 10.)

tensor_float16 = tensor.type(torch.float16)
tensor_int8 = tensor.type(torch.int8)

print(tensor_float16)
print(tensor_int8)
```

Lower precision datatypes can use less memory and compute faster, but they may lose numerical detail.

## Reshaping, Viewing, Stacking, Squeezing, Unsqueezing, and Permuting

These operations change the shape or arrangement of a tensor.

| Method | Purpose |
| --- | --- |
| `torch.reshape(input, shape)` | Reshape to a compatible shape |
| `Tensor.view(shape)` | View the same data in a different shape |
| `torch.stack(tensors, dim=0)` | Stack tensors along a new dimension |
| `torch.squeeze(input)` | Remove dimensions of size 1 |
| `torch.unsqueeze(input, dim)` | Add a dimension of size 1 |
| `torch.permute(input, dims)` | Rearrange dimensions |

### Reshape

```python
x = torch.arange(1., 8.)
x_reshaped = x.reshape(1, 7)

print(x_reshaped)
print(x_reshaped.shape)
```

### View

```python
z = x.view(1, 7)

print(z)
print(z.shape)
```

Important: `view()` shares the same data as the original tensor. Changing the view can change the original.

```python
z[:, 0] = 5

print(z)
print(x)
```

### Stack

```python
x_stacked = torch.stack([x, x, x, x], dim=0)

print(x_stacked)
```

Try changing `dim=0` to `dim=1` and compare the shapes.

### Squeeze

```python
x_reshaped = x.reshape(1, 7)
x_squeezed = x_reshaped.squeeze()

print(x_squeezed)
print(x_squeezed.shape)
```

`squeeze()` removes dimensions with size `1`.

### Unsqueeze

```python
x_unsqueezed = x_squeezed.unsqueeze(dim=0)

print(x_unsqueezed)
print(x_unsqueezed.shape)
```

`unsqueeze()` adds a dimension with size `1`.

### Permute

`permute()` rearranges the order of dimensions.

```python
x_original = torch.rand(size=(224, 224, 3))
x_permuted = x_original.permute(2, 0, 1)

print(f"Previous shape: {x_original.shape}")
print(f"New shape: {x_permuted.shape}")
```

This changes image data from:

```text
[height, width, color_channels]
```

to:

```text
[color_channels, height, width]
```

## Indexing Tensors

Tensor indexing works similarly to Python lists and NumPy arrays.

```python
x = torch.arange(1, 10).reshape(1, 3, 3)

print(x)
print(x.shape)
```

Indexing goes from outer dimension to inner dimension.

```python
print(x[0])
print(x[0][0])
print(x[0][0][0])
```

Use `:` to select all values in a dimension.

```python
print(x[:, 0])
print(x[:, :, 1])
print(x[:, 1, 1])
print(x[0, 0, :])
```

The easiest way to learn indexing is to print shapes often and experiment.

## PyTorch and NumPy

PyTorch works well with NumPy.

Convert a NumPy array to a PyTorch tensor:

```python
import numpy as np

array = np.arange(1.0, 8.0)
tensor = torch.from_numpy(array)

print(array)
print(tensor)
```

NumPy defaults to `float64`, so the resulting tensor will also often be `torch.float64`.

Convert it to `float32` if needed:

```python
tensor = torch.from_numpy(array).type(torch.float32)
```

Convert a PyTorch tensor to a NumPy array:

```python
tensor = torch.ones(7)
numpy_tensor = tensor.numpy()

print(tensor)
print(numpy_tensor)
```

## Reproducibility

Randomness is important in machine learning, but repeatable experiments are also important.

Use `torch.manual_seed()` to make random values reproducible.

```python
RANDOM_SEED = 42

torch.manual_seed(seed=RANDOM_SEED)
random_tensor_C = torch.rand(3, 4)

torch.manual_seed(seed=RANDOM_SEED)
random_tensor_D = torch.rand(3, 4)

print(random_tensor_C)
print(random_tensor_D)
print(random_tensor_C == random_tensor_D)
```

You must reset the seed before each random operation if you want the same random values again.

## Running Tensors on GPUs

Deep learning can involve millions or billions of tensor operations. GPUs are often much faster than CPUs for these operations.

### Check for NVIDIA CUDA GPU

```python
torch.cuda.is_available()
```

Set a device variable:

```python
device = "cuda" if torch.cuda.is_available() else "cpu"
```

Count GPUs:

```python
torch.cuda.device_count()
```

### Check for Apple Silicon GPU

On Apple Silicon Macs, use MPS:

```python
torch.backends.mps.is_available()
```

Device-agnostic code for CUDA, MPS, or CPU:

```python
if torch.cuda.is_available():
    device = "cuda"
elif torch.backends.mps.is_available():
    device = "mps"
else:
    device = "cpu"
```

## Moving Tensors to a Device

Create a tensor on CPU:

```python
tensor = torch.tensor([1, 2, 3])

print(tensor, tensor.device)
```

Move it to the selected device:

```python
tensor_on_gpu = tensor.to(device)

print(tensor_on_gpu)
```

`.to(device)` returns a copy. Reassign if you want the variable to point to the moved tensor.

```python
tensor = tensor.to(device)
```

## Moving Tensors Back to CPU

NumPy cannot directly use tensors stored on GPU. Move the tensor back to CPU first.

```python
tensor_back_on_cpu = tensor_on_gpu.cpu().numpy()

print(tensor_back_on_cpu)
```

The original tensor remains on the GPU unless you reassign it.

## Debugging Checklist

When a PyTorch operation fails, check:

- Shape: `tensor.shape`
- Datatype: `tensor.dtype`
- Device: `tensor.device`

For matrix multiplication, check:

```text
(a, b) @ (c, d)
```

The operation works only if:

```text
b == c
```

The output shape will be:

```text
(a, d)
```

## Key Takeaways

- Tensors are the core data structure in PyTorch.
- Shape tells you how tensor values are arranged.
- Datatype controls how tensor values are stored.
- Device tells you whether the tensor is on CPU, CUDA GPU, or Apple Silicon GPU.
- Matrix multiplication has strict shape rules.
- Most deep learning bugs at this stage come from shape, dtype, or device mismatches.
- `torch.manual_seed()` helps make experiments reproducible.
- Use device-agnostic code so your PyTorch code can run on different machines.

