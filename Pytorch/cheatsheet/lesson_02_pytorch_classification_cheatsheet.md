# Lesson 02: PyTorch Classification (Cheat Sheet)

## 1) Binary vs multiclass (most important)

### Binary classification
- Output layer: `out_features=1`
- Loss: `nn.BCEWithLogitsLoss()`
- Convert logits → probs: `torch.sigmoid(logits)`
- Convert probs → labels: `torch.round(probs)`

### Multiclass classification (K classes)
- Output layer: `out_features=K`
- Loss: `nn.CrossEntropyLoss()`
- **Pass raw logits to loss** (no softmax before loss)
- Pred labels: `torch.softmax(logits, dim=1).argmax(dim=1)`

## 2) Activation quick guide
- `sigmoid`: binary probability style output.
- `softmax`: multiclass probability distribution per row.
- `tanh`: hidden-layer nonlinearity in [-1, 1].
- `relu`: common default hidden nonlinearity.

## 3) Correct metric setup
```python
from torchmetrics import Accuracy

# binary example
acc_bin = Accuracy(task="binary").to(device)

# multiclass example (3 classes)
acc_multi = Accuracy(task="multiclass", num_classes=3).to(device)
```

Use as:
```python
acc = acc_multi(pred_labels, y_true)
```

## 4) Binary loop pattern
```python
logits = model(X).squeeze()
loss = bce_loss(logits, y.float())
preds = torch.round(torch.sigmoid(logits))
```

## 5) Multiclass loop pattern
```python
logits = model(X)                    # [N, K]
loss = ce_loss(logits, y.long())
probs = torch.softmax(logits, dim=1)
preds = torch.argmax(probs, dim=1)
```

## 6) Tanh formula (from exercise)
$$
\tanh(x)=\frac{e^x-e^{-x}}{e^x+e^{-x}}
$$

PyTorch:
```python
y = torch.tanh(x)
```

## 7) Decision boundary interpretation
- Clean, separated regions usually mean the model learned useful nonlinear boundaries.
- High train accuracy + poor test accuracy = overfitting warning.
- Always log both train and test metrics.

## 8) Common classification mistakes
- applying `softmax` before `CrossEntropyLoss`.
- wrong `num_classes` in metric.
- wrong target dtype (`float` for CE targets should be `long`).
- for BCE target shape mismatch (`[N]` vs `[N,1]`).

## 9) 10-second memory
- Binary: BCE + sigmoid + round
- Multiclass: CE + softmax + argmax
- Hidden layers: use nonlinearity (`tanh`/`relu`)