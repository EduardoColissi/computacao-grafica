# Sistema de Trajetórias - M6.cpp

Este projeto implementa um sistema completo de trajetórias para objetos 3D em OpenGL, permitindo criar, editar, salvar e carregar trajetórias cíclicas.


##Video
https://github.com/user-attachments/assets/eb5cdeb0-c297-4057-b829-e38b2cbeb1b9

## Funcionalidades

### Controles Principais
- **T**: Ativar/Desativar modo trajetória
- **P**: Adicionar ponto de controle (teclado)
- **Clique Esquerdo**: Adicionar ponto de controle (mouse)
- **SPACE**: Iniciar/Parar trajetória
- **C**: Limpar trajetória atual
- **S**: Salvar trajetória em arquivo
- **L**: Carregar trajetória de arquivo
- **O**: Selecionar próximo objeto
- **V**: Mostrar/Esconder pontos de controle
- **I**: Mostrar informações da trajetória

### Trajetórias Pré-definidas
- **1**: Criar trajetória circular
- **2**: Criar trajetória quadrada
- **3**: Criar trajetória triangular

### Controles de Câmera
- **WASD**: Movimento da câmera
- **Mouse**: Olhar ao redor
- **ESC**: Sair do programa

## Como Usar

### 1. Criando uma Trajetória Manual
1. Pressione **T** para ativar o modo trajetória
2. Use **WASD** e o mouse para posicionar a câmera
3. Pressione **P** ou clique com o mouse esquerdo para adicionar pontos de controle
4. Pressione **SPACE** para iniciar a trajetória

### 2. Usando Trajetórias Pré-definidas
1. Pressione **T** para ativar o modo trajetória
2. Pressione **1**, **2** ou **3** para criar trajetórias circulares, quadradas ou triangulares
3. Pressione **SPACE** para iniciar a trajetória

### 3. Salvando e Carregando Trajetórias
1. Crie uma trajetória manual ou use uma pré-definida
2. Pressione **S** para salvar em arquivo
3. Pressione **L** para carregar de arquivo

### 4. Visualizando Pontos de Controle
- Pressione **V** para mostrar/esconder os pontos de controle da trajetória
- Os pontos são renderizados como pequenos cubos vermelhos simples

## Formato do Arquivo de Trajetória

O arquivo de trajetória tem o seguinte formato:
```
N
x1 y1 z1 t1
x2 y2 z2 t2
...
xN yN zN tN
```

Onde:
- **N**: Número de pontos de controle
- **xi, yi, zi**: Coordenadas 3D do ponto i
- **ti**: Tempo (em segundos) para chegar ao ponto i

### Exemplo de Arquivo
```
4
0.0 0.0 3.0 2.0
3.0 0.0 0.0 2.0
0.0 0.0 -3.0 2.0
-3.0 0.0 0.0 2.0
```

## Características Técnicas

### Interpolação
- Interpolação linear entre pontos de controle
- Movimento suave e contínuo
- Trajetória cíclica (volta ao primeiro ponto após o último)

### Renderização
- Pontos de controle renderizados como cubos vermelhos pequenos e simples
- Objetos seguem a trajetória em tempo real
- Sistema de iluminação Phong aplicado aos objetos

### Performance
- Atualização baseada em delta time
- Renderização eficiente com VAOs e VBOs
- Suporte a múltiplos objetos na cena

## Estrutura do Código

### Classes Principais
- **ControlPoint**: Representa um ponto de controle com posição e tempo
- **Trajectory**: Gerencia a trajetória completa com pontos de controle
- **SceneObject**: Representa um objeto da cena com geometria e trajetória
- **FirstPersonCamera**: Sistema de câmera em primeira pessoa

### Funcionalidades Implementadas
- Sistema de trajetórias cíclicas
- Adição de pontos via teclado e mouse
- Salvamento/carregamento de arquivos
- Visualização de pontos de controle
- Trajetórias pré-definidas
- Controle de câmera em primeira pessoa
- Sistema de iluminação Phong

## Compilação

Certifique-se de ter as seguintes bibliotecas instaladas:
- GLFW
- GLAD
- GLM
- STB_IMAGE

Compile com:
```bash
g++ -o M6 src/M6.cpp -lglfw -lopengl32 -lgdi32
```

## Exemplo de Uso

1. Execute o programa
2. Pressione **T** para ativar o modo trajetória
3. Pressione **1** para criar uma trajetória circular
4. Pressione **SPACE** para iniciar o movimento
5. Observe o objeto Suzanne seguindo a trajetória circular
6. Pressione **V** para ver os pontos de controle
7. Pressione **I** para ver informações da trajetória 


Desenvolvido por Eduardo Colissi.
