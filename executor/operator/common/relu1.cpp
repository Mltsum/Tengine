#include <iostream>
#include <functional>
#include <cstring>
#include <cmath>

#include "logger.hpp"
#include "node_ops.hpp"
#include "tensor_mem.hpp"
#include "graph.hpp"
#include "data_type.hpp"

namespace TEngine
{

namespace ReLU1Impl
{

struct ReLU1Ops : public NodeOps
{
    bool OnBind(Node* node) override
    {
        inplace_t io_map;

        io_map[0] = 0;
        
        node->SetAttr(ATTR_INPLACE, io_map);

        return true;
    } 

    template <typename data_type> void kernel_run(void* data, data_type* out_data,int size)
    {
        data_type* in_data = ( data_type* )data;

        for(int i = 0; i < size; i++)
        {
            out_data[i] = std::max(( data_type)-1.0,std::min(in_data[i],( data_type )1.0));
        }
    }
    
    template <typename data_type> void print_input(void* data, int size)
    {
        data_type* out_data = ( data_type* )data;
        for(int i = 0; i < size; i++)
        {
            std::cout<<*(out_data++)<<" ";
        }
        std::cout<<std::endl;
    }

    bool Run(Node* node) override
    {
        std::cout<<"start run relu1"<<std::endl;
        Tensor* input_tensor = node->GetInputTensor(0);
        Tensor* output_tensor = node->GetOutputTensor(0);
        int element_size = DataType::GetTypeSize(input_tensor->GetDataType());
        const TShape& shape = input_tensor->GetShape();
        int elem_num = shape.GetSize();
        
        void* in_data = get_tensor_mem(input_tensor);
        void* out_data = get_tensor_mem(output_tensor);
        switch (element_size)
        {
            case 4:
                kernel_run<float>(in_data,(float*)out_data, elem_num);
                break;
#ifdef CONFIG_FLOAT16
            case 2:
                kernel_run<__fp16>(in_data, (__fp16*)out_data, elem_num);
                break;
#endif
            case 1:
                kernel_run<char>(in_data, (char*)out_data, elem_num);
                break;        
            default:
                break;
        }

        return true;
    }
};

NodeOps* SelectFunc(const CPUInfo* cpu_info, Node* node)
{
    Tensor* input = node->GetInputTensor(0);
    const int data_type = input->GetDataType();
    const ExecAttr* exec_attr = any_cast<const ExecAttr*>(node->GetAttr(ATTR_EXEC_ATTR));
    if(data_type != TENGINE_DT_FP32 || exec_attr->graph_layout != TENGINE_LAYOUT_NHWC)
        return nullptr;

    ReLU1Ops* ops = new ReLU1Ops();

    return ops;
}

}

using namespace ReLU1Impl;

void RegisterReLU1NodeExec(void)
{
    NodeOpsRegistryManager::RegisterOPImplementor("common", "ReLU1", ReLU1Impl::SelectFunc, 1000);
}

} // namespace TEngine
