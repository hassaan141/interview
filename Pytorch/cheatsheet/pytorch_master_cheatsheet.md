# PyTorch Master Cheatsheet (Lessons 00-03)

## Contents
1. Fundamentals (Lesson 00)
2. Workflow (Lesson 01)
3. Classification (Lesson 02)
4. Computer Vision (Lesson 03)
5. Visualization (Matplotlib + Confusion Matrix)
6. Saving and Loading Weights

## Lesson References (Notes Folder)

Use these notebooks as the primary source of truth for Lessons 00-03:

1. Lesson 00: `Pytorch/notes/00_pytorch_fundamentals.ipynb`
2. Lesson 01: `Pytorch/notes/01_pytorch_workflow.ipynb`
3. Lesson 02: `Pytorch/notes/02_pytorch_classification.ipynb`
4. Lesson 03: `Pytorch/notes/03_pytorch_computer_vision.ipynb`

---

## 1) Fundamentals (Lesson 00)

### Big picture
- PyTorch works with tensors (like NumPy arrays + GPU + autograd).
- Typical loop:
  1. Create data as tensors
  2. Build model
  3. Compute loss
  4. Backprop (`loss.backward()`)
  5. Update params (`optimizer.step()`)

### Tensor basics
```python
import torch

x = torch.tensor([1., 2., 3.])
A = torch.rand(3, 4)
Z = torch.zeros((2, 2))
O = torch.ones((2, 2))

print(x.shape, x.dtype, x.device)
```

### dtype and device
```python
x = x.type(torch.float32)
device = "cuda" if torch.cuda.is_available() else "cpu"
x = x.to(device)
```

### Core ops
```python
a = torch.tensor([1., 2., 3.])
b = torch.tensor([4., 5., 6.])

a + b
a * b
torch.matmul(torch.rand(2, 3), torch.rand(3, 4))
a.mean(), a.sum(), a.max(), a.min(), a.argmax()
```

### NumPy <-> PyTorch
```python
import numpy as np

arr = np.array([1, 2, 3])
t = torch.from_numpy(arr).float()
arr2 = t.cpu().numpy()
```

### Reproducibility
```python
torch.manual_seed(42)
```

### Autograd idea
```python
w = torch.tensor(2.0, requires_grad=True)
y = w * 3
loss = (y - 10) ** 2
loss.backward()
print(w.grad)
```

### Common mistakes
- dtype mismatch
- device mismatch
- shape mismatch
- no seed when comparing runs

---

## 2) Workflow (Lesson 01)

### End-to-end pipeline
1. Prepare data
2. Build model (`nn.Module`)
3. Choose loss + optimizer
4. Train loop (`model.train()`)
5. Eval loop (`model.eval()` + `torch.inference_mode()`)
6. Save/load model

### Skeleton
```python
import torch
from torch import nn

class ModelV0(nn.Module):
    def __init__(self):
        super().__init__()
        self.layer = nn.Linear(in_features=1, out_features=1)

    def forward(self, x):
        return self.layer(x)

model = ModelV0().to(device)
loss_fn = nn.L1Loss()
optimizer = torch.optim.SGD(model.parameters(), lr=0.01)
```

### Train/eval template
```python
for epoch in range(epochs):
    model.train()
    y_pred = model(X_train)
    loss = loss_fn(y_pred, y_train)

    optimizer.zero_grad()
    loss.backward()
    optimizer.step()

    model.eval()
    with torch.inference_mode():
        test_pred = model(X_test)
        test_loss = loss_fn(test_pred, y_test)
```

### Save/load
```python
torch.save(model.state_dict(), "model_v0.pth")

loaded = ModelV0()
loaded.load_state_dict(torch.load("model_v0.pth"))
loaded.eval()
```

### Common bugs
- model/data on different devices
- forgetting `optimizer.zero_grad()`
- evaluating without `model.eval()`
- shape mismatch in targets and predictions

---

## 3) Classification (Lesson 02)

### Binary vs multiclass

#### Binary classification
- Output layer: `out_features=1`
- Loss: `nn.BCEWithLogitsLoss()`
- Convert logits -> probs: `torch.sigmoid(logits)`
- Convert probs -> labels: `torch.round(probs)`

#### Multiclass classification (K classes)
- Output layer: `out_features=K`
- Loss: `nn.CrossEntropyLoss()`
- Pass raw logits to loss (no softmax before loss)
- Pred labels: `torch.argmax(logits, dim=1)`

### Activation quick guide
- sigmoid: binary probability style output
- softmax: multiclass distribution
- tanh: hidden nonlinearity in [-1, 1]
- relu: common hidden nonlinearity

### Accuracy helpers (`torchmetrics`)
```python
from torchmetrics import Accuracy

acc_bin = Accuracy(task="binary").to(device)
acc_multi = Accuracy(task="multiclass", num_classes=3).to(device)
```

### Binary loop pattern
```python
logits = model(X).squeeze()
loss = bce_loss(logits, y.float())
preds = torch.round(torch.sigmoid(logits))
```

### Multiclass loop pattern
```python
logits = model(X)
loss = ce_loss(logits, y.long())
preds = torch.argmax(logits, dim=1)
```

### Tanh formula
$$
\tanh(x)=\frac{e^x-e^{-x}}{e^x+e^{-x}}
$$

### Common mistakes
- applying softmax before `CrossEntropyLoss`
- wrong `num_classes` in metric
- wrong target dtype
- BCE shape mismatch (`[N]` vs `[N,1]`)

---

## 4) Computer Vision (Lesson 03)

### Core CV libraries
- `torchvision`: datasets, models, transforms
- `torchvision.datasets`: ready datasets (MNIST, FashionMNIST, ...)
- `torchvision.models`: pretrained CV models
- `torchvision.transforms`: preprocessing/augmentation
- `torch.utils.data.Dataset` and `DataLoader`: data pipelines

### CNN vs MLP input (MNIST)
- MNIST sample: `[1, 28, 28]`
- `1` = channels (grayscale)
- `28 x 28` = spatial layout

#### CNN uses channels
`nn.Conv2d` expects `[batch, channels, height, width]`.
For MNIST, first conv uses `in_channels=1`.

#### MLP uses flattened features
`nn.Linear` expects 1D vectors per sample.
Flattened MNIST size: `1 x 28 x 28 = 784`.
So first linear uses `in_features=784`.

### Quick shape flow
- Raw sample: `[1, 28, 28]`
- CNN batch: `[32, 1, 28, 28]`
- Flattened MLP batch: `[32, 784]`

Rule of thumb:
- `Conv2d`: keep channels and spatial dims
- `Linear`: flatten into feature vectors

### What `Conv2d` and `MaxPool2d` do

#### `nn.Conv2d`
- Slides learned filters over local patches (for example `3 x 3`)
- Learns edges, curves, strokes, textures
- Produces feature maps showing where patterns appear

#### `nn.MaxPool2d`
- No learned weights
- Downsamples by taking max in local windows
- With `kernel_size=2, stride=2`: halves height and width
- Keeps strongest activations, drops weaker/noisy responses

Example shape:
- Before pool: `[1, 10, 28, 28]`
- After `MaxPool2d(2,2)`: `[1, 10, 14, 14]`

Common stack:
1. `Conv2d`
2. `ReLU`
3. `MaxPool2d`

### Why logits/preds/labels felt more visible in Lesson 2
- You still need logits, labels, and preds for image classification.
- In CV notebooks, early focus is often data shapes and CNN blocks, so that part can feel less explicit at first.

Definitions:
- logits: raw model outputs
- labels: ground-truth class ids
- preds: predicted class ids

Important rules:
- For `nn.CrossEntropyLoss`: use logits + labels directly.
- For accuracy/confusion matrix: `preds = torch.argmax(logits, dim=1)`.

So CV does use logits/preds/labels too; they just appear later while building the pipeline.

### Why we do y_pred = test_pred.argmax(dim=1)

This line converts model scores into class labels.

- `test_pred` is usually shape `[batch_size, num_classes]`.
- Each row contains one score per class (these are logits).
- The biggest score in a row is the model's chosen class.

`argmax(dim=1)` means:

- look across classes for each image
- return the index of the biggest score

That index is the predicted label for that image.

Simple example (3 images, 4 classes):

- logits row 1: `[0.1, 2.4, -0.7, 1.2]` -> predicted class `1`
- logits row 2: `[3.0, 0.2, 0.1, -1.0]` -> predicted class `0`
- logits row 3: `[-0.5, 0.0, 0.9, 0.4]` -> predicted class `2`

So `argmax(dim=1)` gives `preds = [1, 0, 2]`.

Why this is needed for accuracy:

- accuracy compares predicted class ids with true class ids
- true labels are integers like `[1, 0, 2]`
- logits are not class ids, they are raw scores

So the flow is:

1. model outputs logits
2. `argmax(dim=1)` converts logits -> predicted labels
3. compare predicted labels with true labels for accuracy

Memory trick:

- logits = "how much each class fits"
- argmax = "pick the winner class"
- accuracy = "did winner match truth"

### Why we sum batch metrics then divide by `len(data_loader)`

When you loop over a dataloader, you get one batch at a time. So you usually:

1. compute metric per batch
2. add it to a running total
3. divide by number of batches at the end

That gives the mean metric across batches.

What is `len(data_loader)`?

- It is the number of batches, not the number of samples.
- Roughly: `ceil(num_samples / batch_size)`.

Example A (exactly divisible):

- `num_samples = 60000`, `batch_size = 32`
- `len(data_loader) = ceil(60000 / 32) = 1875` batches

If per-batch losses sum to `356.25`, then:

- `avg_loss = 356.25 / 1875 = 0.19`

Example B (last batch smaller):

- `num_samples = 100`, `batch_size = 32`
- Batch sizes are `[32, 32, 32, 4]`
- `len(data_loader) = 4`

If batch accuracies are `[0.75, 0.81, 0.78, 1.00]`, simple batch mean is:

- `(0.75 + 0.81 + 0.78 + 1.00) / 4 = 0.835`

Important note:

- Dividing by `len(data_loader)` is the average of batch metrics.
- For most accurate dataset-level accuracy, use sample-weighted accuracy:
    `total_correct / total_samples`.
- For loss, many people still average batch losses with `len(data_loader)`, which is usually fine when batch sizes are mostly equal.

---

## Fast Evaluation Snippet (multiclass)

```python
model.eval()
correct = 0
total = 0
test_loss = 0.0

with torch.inference_mode():
    for X, y in test_dataloader:
        X, y = X.to(device), y.to(device)
        logits = model(X)
        loss = loss_fn(logits, y)
        test_loss += loss.item()

        preds = torch.argmax(logits, dim=1)
        correct += (preds == y).sum().item()
        total += y.size(0)

avg_test_loss = test_loss / len(test_dataloader)
test_acc = correct / total
print(f"test_loss={avg_test_loss:.4f}, test_acc={test_acc:.4f}")
```

---

## 5) Visualization (Matplotlib + Confusion Matrix)

Visualization is a core skill in PyTorch workflows. Use it to inspect:

1. data quality
2. class balance
3. model predictions
4. mistakes and failure patterns

### A) Show one image

```python
import matplotlib.pyplot as plt

image, label = train_data[0]        # image shape: [1, 28, 28]
plt.imshow(image.squeeze(), cmap="gray")
plt.title(f"Label: {train_data.classes[label]}")
plt.axis(False)
```

### B) Show a grid of random samples

```python
import torch
import matplotlib.pyplot as plt

torch.manual_seed(42)
fig = plt.figure(figsize=(8, 8))
rows, cols = 4, 4

for i in range(1, rows * cols + 1):
    idx = torch.randint(0, len(train_data), size=[1]).item()
    img, label = train_data[idx]
    fig.add_subplot(rows, cols, i)
    plt.imshow(img.squeeze(), cmap="gray")
    plt.title(train_data.classes[label])
    plt.axis(False)
```

### C) Plot training curves (loss/accuracy vs epoch)

```python
epochs = [1, 2, 3]
train_losses = [1.42, 0.52, 0.42]
test_losses  = [0.60, 0.44, 0.38]
train_accs   = [53.9, 83.9, 87.5]
test_accs    = [81.2, 86.1, 88.3]

fig, ax = plt.subplots(1, 2, figsize=(12, 4))

ax[0].plot(epochs, train_losses, label="train loss")
ax[0].plot(epochs, test_losses, label="test loss")
ax[0].set_title("Loss")
ax[0].set_xlabel("Epoch")
ax[0].legend()

ax[1].plot(epochs, train_accs, label="train acc")
ax[1].plot(epochs, test_accs, label="test acc")
ax[1].set_title("Accuracy (%)")
ax[1].set_xlabel("Epoch")
ax[1].legend()
```

### D) Visualize predictions vs true labels

```python
model.eval()
fig = plt.figure(figsize=(10, 6))
rows, cols = 2, 5

with torch.inference_mode():
    for i in range(1, rows * cols + 1):
        idx = torch.randint(0, len(test_data), size=[1]).item()
        img, y_true = test_data[idx]
        logits = model(img.unsqueeze(0).to(device))
        y_pred = logits.argmax(dim=1).item()

        fig.add_subplot(rows, cols, i)
        plt.imshow(img.squeeze(), cmap="gray")
        color = "green" if y_pred == y_true else "red"
        plt.title(f"P:{y_pred} / T:{y_true}", color=color)
        plt.axis(False)
```

### E) Confusion matrix (very important)

Use this to see which classes get confused with each other.

```python
from torchmetrics import ConfusionMatrix
import matplotlib.pyplot as plt

confmat = ConfusionMatrix(task="multiclass", num_classes=len(train_data.classes)).to(device)

all_preds, all_targets = [], []
model.eval()
with torch.inference_mode():
    for X, y in test_dataloader:
        X, y = X.to(device), y.to(device)
        logits = model(X)
        preds = logits.argmax(dim=1)
        all_preds.append(preds)
        all_targets.append(y)

all_preds = torch.cat(all_preds)
all_targets = torch.cat(all_targets)
cm = confmat(all_preds, all_targets).cpu().numpy()

fig, ax = plt.subplots(figsize=(8, 7))
im = ax.imshow(cm, cmap="Blues")
ax.set_title("Confusion Matrix")
ax.set_xlabel("Predicted")
ax.set_ylabel("True")
ax.set_xticks(range(len(train_data.classes)))
ax.set_yticks(range(len(train_data.classes)))
ax.set_xticklabels(train_data.classes, rotation=45, ha="right")
ax.set_yticklabels(train_data.classes)
fig.colorbar(im)
plt.tight_layout()
```

How to read confusion matrix:

1. diagonal cells = correct predictions
2. off-diagonal cells = mistakes
3. bright off-diagonal regions show specific class confusions

---

## 6) Saving and Loading Weights

Always save model weights using `state_dict()`.

### A) Save weights

```python
from pathlib import Path
import torch

MODEL_PATH = Path("models")
MODEL_PATH.mkdir(parents=True, exist_ok=True)

MODEL_NAME = "tinyvgg_mnist.pth"
MODEL_SAVE_PATH = MODEL_PATH / MODEL_NAME

torch.save(model.state_dict(), MODEL_SAVE_PATH)
print(f"Saved to: {MODEL_SAVE_PATH}")
```

### B) Load weights

```python
# Recreate architecture first
loaded_model = TinyVGG(input=1, hidden=10, output=len(train_data.classes)).to(device)

# Load weights
loaded_model.load_state_dict(torch.load(MODEL_SAVE_PATH, map_location=device))
loaded_model.eval()
```

### C) Quick sanity check after loading

```python
model.eval()
loaded_model.eval()

with torch.inference_mode():
    sample_x, _ = test_data[0]
    sample_x = sample_x.unsqueeze(0).to(device)
    original_out = model(sample_x)
    loaded_out = loaded_model(sample_x)

print(torch.allclose(original_out, loaded_out, atol=1e-6))
```

Best practices:

1. save only `state_dict()`, not full model object
2. recreate model class before loading weights
3. call `.eval()` for inference
4. use `map_location=device` when loading across cpu/gpu
5. keep a clear model naming convention (dataset + architecture + date/version)

## 10-second memory
- Loss tells how wrong/confident the model is.
- Accuracy tells how often predictions are correct.
- Track both on train and test.
