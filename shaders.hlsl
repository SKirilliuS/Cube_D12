// ============================================================================
// НАСТРОЙКИ И ВХОДНЫЕ ДАННЫЕ ШЕЙДЕРА (СТАНДАРТ HLSL 5.1 ДЛЯ DIRECTX 12)
// ============================================================================

// В HLSL 5.1 константы упаковываются в явную структуру.
// Ключевое слово row_major гарантирует, что строки из C++ лога прочитаются как строки.
struct MatrixContainer
{
    row_major float4x4 wvpMatrix;
};

// Привязываем структуру корневых констант к регистру b0
ConstantBuffer<MatrixContainer> RootConstants : register(b0);

// Структура данных, поступающих ИЗ C++ кода (Vertex Buffer) В вершинный шейдер.
struct VSInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

// Структура данных, передаваемых ИЗ вершинного шейдера В пиксельный шейдер.
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

// ============================================================================
// ВЕРШИННЫЙ ШЕЙДЕР (VERTEX SHADER)
// ============================================================================
PSInput VSMain(VSInput input)
{
    PSInput output;

    // Переводим 3D координаты вершины (X, Y, Z) в однородные 4D координаты
    float4 localPos = float4(input.position, 1.0f);

    // Обращаемся к матрице внутри нашей структуры RootConstants.
    // Порядок умножения вектор-строки на Row-Major матрицу: mul(вектор, матрица)
    output.position = mul(localPos, RootConstants.wvpMatrix);

    // Передаем цвет вершин без изменений
    output.color = input.color;

    return output;
}

// ============================================================================
// ПИКСЕЛЬНЫЙ ШЕЙДЕР (PIXEL SHADER)
// ============================================================================
float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
