import torch
import onnx 
from ultralytics import YOLO 
import argparse 
from pathlib import Path

# Parse command line arguments
parser = argparse.ArgumentParser(description='Export YOLOv8 PyTorch model to ONNX format')
parser.add_argument("--model", type=str, required=True, help="Path to the YOLOv8 PyTorch model file")
parser.add_argument("--output", type=str, required=True, help="Path to save the exported ONNX model")
args = parser.parse_args()

# validate paths
model_path = Path(args.model)
if not model_path.is_file():
    print(f"Error: The specified model file '{args.model}' does not exist.")
    exit(1)
output_dir = Path(args.output)
# Check whether output directory exists, if not create it
if not output_dir.parent.exists():
    output_dir.parent.mkdir(parents=True, exist_ok=True)

onnx_filename = model_path.stem + ".onnx"
onnx_output_path = output_dir / onnx_filename

# Load the Pytorch model
yolo = YOLO(args.model)
pt_model = yolo.model
pt_model.eval()

# Wrapper(remove dict output) for ONNX export
class ModelWrapper(torch.nn.Module):
    def  __init__(self, model):
        super(ModelWrapper, self).__init__()
        self.model = model 

    def forward(self, x):
        outputs = self.model(x)
        if isinstance(outputs, (list, tuple)):
            return outputs[0]  # Return the first element if it's a list or tuple
        return outputs

model_wrapped = ModelWrapper(pt_model)
model_wrapped.eval()

# Create dummy input for ONNX export
dummy_input = torch.randn(1, 3, 640, 640)  #

torch.onnx.export(model_wrapped, dummy_input, onnx_output_path, 
                  export_params=True, opset_version=18,
                  input_names=['images'], output_names=['output'],
                  dynamic_axes=None,
                  dynamo=False)

print(f"Model exported to {onnx_output_path} successfully.")

# Validate the exported ONNX model
onnx_model = onnx.load(onnx_output_path)
onnx.checker.check_model(onnx_model)
print("ONNX model is valid.")

# Simplify onnx
from onnxsim import simplify
# Save raw model (already saved, just for clarity)
raw_path = onnx_output_path

# Create simplified model path
simplified_path = onnx_output_path.with_name(onnx_output_path.stem + "_simplified.onnx")

simplified_model, check = simplify(onnx_model)
if check:
    onnx.save(simplified_model, simplified_path)
    print(f"Simplified ONNX model saved to {simplified_path} successfully.")

model1 = onnx.load(onnx_output_path)
model2 = onnx.load(simplified_path)

print("Original nodes:", len(model1.graph.node))
print("Simplified nodes:", len(model2.graph.node))