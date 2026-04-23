struct VS_INPUT
{
    float3 pos : POSITION;
    float4 col : COLOR; // 텍스처의 어디를 읽을지 나타내는 0~1 사이 좌표
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

PS_INPUT VS_Main(VS_INPUT input)
{
    PS_INPUT output;
    
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;

    return output;
}

float4 PS_Main(PS_INPUT input) : SV_Target
{
    return float4(1, 1, 1, 0);
}