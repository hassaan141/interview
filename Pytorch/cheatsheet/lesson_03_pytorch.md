# DATA LOADING
# DATA PREPPIGN

## 0. Computer vision libraries in PyTorch

Before we get started writing code, let's talk about some PyTorch computer vision libraries you should be aware of.

| PyTorch module | What does it do? |
| ----- | ----- |
| [`torchvision`](https://pytorch.org/vision/stable/index.html) | Contains datasets, model architectures and image transformations often used for computer vision problems. |
| [`torchvision.datasets`](https://pytorch.org/vision/stable/datasets.html) | Here you'll find many example computer vision datasets for a range of problems from image classification, object detection, image captioning, video classification and more. It also contains [a series of base classes for making custom datasets](https://pytorch.org/vision/stable/datasets.html#base-classes-for-custom-datasets). |
| [`torchvision.models`](https://pytorch.org/vision/stable/models.html) | This module contains well-performing and commonly used computer vision model architectures implemented in PyTorch, you can use these with your own problems. |
| [`torchvision.transforms`](https://pytorch.org/vision/stable/transforms.html) | Often images need to be transformed (turned into numbers/processed/augmented) before being used with a model, common image transformations are found here. |
| [`torch.utils.data.Dataset`](https://pytorch.org/docs/stable/data.html#torch.utils.data.Dataset) | Base dataset class for PyTorch.  |
| [`torch.utils.data.DataLoader`](https://pytorch.org/docs/stable/data.html#module-torch.utils.data) | Creates a Python iterable over a dataset (created with `torch.utils.data.Dataset`). |

> **Note:** The `torch.utils.data.Dataset` and `torch.utils.data.DataLoader` classes aren't only for computer vision in PyTorch, they are capable of dealing with many different types of data.

Now we've covered some of the most important PyTorch computer vision libraries, let's import the relevant dependencies.

## CNN vs MLP input cheatsheet (MNIST example)

MNIST sample shape is `[1, 28, 28]`.

- `1` is channels (grayscale)
- `28 x 28` is spatial layout (height x width)

### Why CNN uses `in_channels=1`

`nn.Conv2d` expects input as `[batch, channels, height, width]`.
For MNIST, a batch is `[N, 1, 28, 28]`, so the first conv layer must use `in_channels=1`.

CNN intuition: keep spatial structure so filters can learn local patterns such as edges, corners, and strokes.

### Why MLP uses `in_features=784`

`nn.Linear` expects a 1D feature vector per sample.
So the image is flattened: `1 x 28 x 28 = 784`.
After flattening, batch shape is `[N, 784]`, so the first linear layer uses `in_features=784`.

MLP intuition: works on a list of values and does not preserve explicit 2D neighborhood structure after flattening.

### Quick shape flow

- Raw sample: `[1, 28, 28]`
- CNN batch: `[32, 1, 28, 28]`
- Flattened MLP batch: `[32, 784]`

Rule of thumb:

- If layer is `Conv2d`, send channels and spatial dims.
- If layer is `Linear`, flatten first and send feature vectors.

## What `Conv2d` and `MaxPool2d` do (simple intuition)

### `nn.Conv2d`

Your intuition is correct: a Conv2d filter slides over the image and learns useful patterns.

- It looks at small local patches (for example `3 x 3`).
- Each filter learns to respond to a pattern (edge, curve, stroke, texture).
- The output is a feature map showing where that pattern appears strongly.

Think of Conv2d as a "pattern scanner" moving across the image.

### `nn.MaxPool2d`

MaxPool does not learn new filters. It downsamples feature maps by taking the maximum value in each local window.

- Example: with `kernel_size=2, stride=2`, each `2 x 2` region becomes 1 value (the max).
- This reduces height and width (fewer computations, less memory).
- Strong activations are kept, weaker/noisy ones are dropped.

Think of MaxPool as "keep the strongest signal in each neighborhood."

### Tiny example

If a feature map is shape `[1, 10, 28, 28]`:

- After `Conv2d` with `padding=1, stride=1, kernel_size=3`, spatial size can stay `28 x 28`.
- After `MaxPool2d(kernel_size=2, stride=2)`, it becomes `[1, 10, 14, 14]`.

So a common pattern is:

1. `Conv2d` to detect patterns
2. `ReLU` for non-linearity
3. `MaxPool2d` to compress while keeping strong signals

## Why logits/preds/labels seemed less explicit here than lesson 2

Short version: you still need them for image **classification**. In this notebook, sometimes you were focusing on data shapes and architecture first, so those steps were less visible.

### What each one is

- `logits`: raw model outputs before converting to probabilities
- `labels`: ground-truth class ids from the dataset
- `preds`: predicted class ids (usually from `argmax`)

### Why lesson 2 emphasized them

Lesson 2 focused directly on classification math and metrics, so the conversion pipeline was explicit:

- Binary: logits -> sigmoid -> round -> preds
- Multiclass: logits -> softmax/argmax (or just argmax) -> preds

### Why lesson 3 can feel different

In computer vision, early steps often focus on:

- image tensors and shapes
- CNN blocks (`Conv2d`, `MaxPool2d`)
- data loading and transforms

So the logits/preds part can appear later in training/evaluation cells.

### Important rule

- For `nn.CrossEntropyLoss`: pass `logits` and `labels` directly to loss.
- For accuracy/confusion matrix: convert logits to `preds` using `argmax(dim=1)`.

So it is not that CV does not use logits/preds/labels. It does. They may just be less front-and-center while you are building the CNN pipeline.
