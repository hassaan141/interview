# Lesson 00: PyTorch Fundamentals (Cheat Sheet)

## 1) Big picture
- PyTorch works with **tensors** (like NumPy arrays + GPU + autograd).
- Most deep learning flow:
  1. Create data as tensors
  2. Build model
  3. Compute loss
  4. Backprop (`loss.backward()`)
  5. Update params (`optimizer.step()`)

## 2) Tensors you must know
```python
import torch

x = torch.tensor([1., 2., 3.])
A = torch.rand(3, 4)
Z = torch.zeros((2, 2))
O = torch.ones((2, 2))
```

### Shape, dtype, device
```python
x.shape
x.dtype
x.device
```

### Convert dtype/device
```python
x = x.type(torch.float32)
device = "cuda" if torch.cuda.is_available() else "cpu"
x = x.to(device)
```

## 3) Tensor ops (daily use)
```python
a = torch.tensor([1., 2., 3.])
b = torch.tensor([4., 5., 6.])

# elementwise
a + b
a * b

# matrix multiply
torch.matmul(torch.rand(2, 3), torch.rand(3, 4))

# reductions
a.mean(), a.sum(), a.max(), a.min(), a.argmax()
```

## 4) NumPy ↔ PyTorch
```python
import numpy as np

arr = np.array([1, 2, 3])
t = torch.from_numpy(arr).float()
arr2 = t.cpu().numpy()
```

## 5) Reproducibility
```python
RANDOM_SEED = 42
torch.manual_seed(RANDOM_SEED)
```

## 6) Autograd (core idea)
- If tensor has `requires_grad=True`, PyTorch tracks operations.
- Call `loss.backward()` to compute gradients.

```python
w = torch.tensor(2.0, requires_grad=True)
y = w * 3
loss = (y - 10) ** 2
loss.backward()
print(w.grad)
```

## 7) Most common mistakes
- dtype mismatch (model expects float inputs).
- device mismatch (CPU tensor with CUDA model).
- shape mismatch (especially labels vs predictions).
- forgetting seed when comparing runs.

## 8) 10-second memory
- Tensor = data
- Autograd = gradients
- Device = cpu/cuda
- dtype + shape + device must be correct