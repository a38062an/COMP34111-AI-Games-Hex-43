import torch 
import torch.nn as nn
import torchvision.ops

class ResidualBlock(nn.Module):
    """
    Residual block with two convolutional layers, batch normalization, ReLU activation, 
    and a Squeeze-and-Excitation block, followed by a skip connection.

    Input:
        x (torch.Tensor): Input tensor of shape (batch_size, 64, H, W).
    
    Output:
        torch.Tensor: Output tensor of shape (batch_size, 64, H, W).

    """
    def __init__(self):
        super(ResidualBlock, self).__init__()
        self.conv1 = nn.Conv2d(32, 32, kernel_size=3, padding=1)
        self.conv2 = nn.Conv2d(32, 32, kernel_size=3, padding=1)
        self.bn1 = nn.BatchNorm2d(32)
        self.bn2 = nn.BatchNorm2d(32)
        self.relu = nn.ReLU()
        self.se_block = torchvision.ops.SqueezeExcitation(32, 16) # Reduction ratio of 2 to preserve info
    
    def forward(self, x):
        out = self.conv1(x)
        out = self.bn1(out)
        out = self.relu(out)
        out = self.conv2(out)
        out = self.bn2(out)
        out = self.se_block(out)
        out = out + x # add input to output
        out = self.relu(out)
        return out

class HexModel(nn.Module):
    """
    MiniResNet-4 for Hex policy prediction.
    
    Input: (batch, 6, 11, 11) - board features
    Output: (batch, 121) - move logits (unnormalised log-probabilities)
    """
    def __init__(self):
        super(HexModel, self).__init__()
        self.stem = nn.Sequential(
            nn.Conv2d(6, 32, kernel_size=3, padding=1),
            nn.BatchNorm2d(32),
            nn.ReLU(),
        )
        self.residuals = nn.Sequential(
            ResidualBlock(),
            ResidualBlock(),
            ResidualBlock(),
            ResidualBlock(),
            ResidualBlock(),
            ResidualBlock()
        )

        self.policy_head = nn.Sequential(
            nn.Conv2d(32, 2, kernel_size=1),
            nn.BatchNorm2d(2),
            nn.ReLU(),
            nn.Flatten(),
            nn.Linear(2 * 11 * 11, 121),
        )

        self.value_head = nn.Sequential(
            nn.Conv2d(32, 1, kernel_size=1),
            nn.BatchNorm2d(1),
            nn.ReLU(),
            nn.Flatten(),
            nn.Linear(1 * 11 * 11, 32),
            nn.ReLU(),
            nn.Linear(32, 1),
            nn.Tanh() # Value is [-1, 1]
        )

    def forward(self, x):
        x = self.stem(x)
        x = self.residuals(x)
        policy = self.policy_head(x)
        value = self.value_head(x)
        return policy, value

