# Lesson 01: PyTorch Workflow (Cheat Sheet)

## 1) End-to-end pipeline
1. Prepare data (train/test split)
2. Build model (`nn.Module`)
3. Choose loss + optimizer
4. Train loop (`model.train()`)
5. Eval loop (`model.eval()` + `torch.inference_mode()`)
6. Save/load model

## 2) Typical skeleton
```python
import torch
from torch import nn
from sklearn.model_selection import train_test_split

# data
X_train, X_test, y_train, y_test = train_test_split(X, y, train_size=0.8, random_state=42)

# model
class ModelV0(nn.Module):
    def __init__(self):
        super().__init__()
        self.layer = nn.Linear(in_features=1, out_features=1)

    def forward(self, x):
        return self.layer(x)

model = ModelV0().to(device)

# loss + optimizer
loss_fn = nn.L1Loss()   # or nn.MSELoss()
optimizer = torch.optim.SGD(model.parameters(), lr=0.01)
```

## 3) Training loop template
```python
epochs = 200
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

## 4) Why train/eval mode matters
- `model.train()` enables training behavior (dropout/batchnorm updates).
- `model.eval()` switches to inference behavior.
- `torch.inference_mode()` disables grad tracking for speed/memory.

## 5) Save and load
```python
# save
torch.save(model.state_dict(), "model_v0.pth")

# load
loaded = ModelV0()
loaded.load_state_dict(torch.load("model_v0.pth"))
loaded.eval()
```

## 6) Common bugs in workflow
- not moving model/data to same device.
- forgetting `optimizer.zero_grad()`.
- evaluating without `model.eval()`.
- comparing tensors with mismatched shapes (e.g., `[N,1]` vs `[N]`).

## 7) 10-second memory
- Build → Loss → Optimize → Evaluate
- Use `train()` during learning, `eval()` during testing
- Save/load with `state_dict()`