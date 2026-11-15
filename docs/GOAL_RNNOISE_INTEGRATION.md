# Goal — Integração correta do RNNoise no NOVOX

**Branch de trabalho:** dev/yin-midi  
**Baseline confirmado:** d1439617a5992035be0bcd2691a006f4597b8606  
**Escopo:** substituir o redutor de ruído tipo gate por RNNoise real, mantendo intacto o fluxo de detecção de pitch YIN e geração MIDI.

Este documento é a especificação de trabalho e de aceite. A implementação só estará concluída quando todos os critérios abaixo forem atendidos e a CI produzir um executável utilizável.

## 1. Objetivo do produto

Integrar o RNNoise oficial da Xiph como redução de ruído neural opcional para a entrada vocal do NOVOX. O processamento deve melhorar a relação voz/ruído antes da detecção de pitch, sem degradar desnecessariamente a fundamental, os harmônicos e os ataques da voz.

O recurso deve:
- permanecer opcional, com bypass transparente quando desativado;
- não expor controles fictícios: a API RNNoise não oferece parâmetro direto de intensidade;
- funcionar nas taxas de amostragem suportadas pelo dispositivo de áudio;
- aceitar blocos de áudio de tamanhos variáveis;
- executar exclusivamente no worker de processamento, nunca no callback de áudio do dispositivo;
- preservar a arquitetura, os controles e os comportamentos atuais de pitch/MIDI.

## 2. Fonte e licenciamento

- Projeto upstream: https://github.com/xiph/rnnoise
- Projeto oficial em C, licenciado sob BSD-3-Clause.
- Fixar uma versão/tag ou commit imutável; não depender de main flutuante.
- Registrar a versão/commit escolhido e incluir os avisos/licença BSD-3-Clause exigidos.
- Não copiar código de forks ou binários pré-compilados sem procedência.
- Antes de integrar, conferir a API e a lista real de fontes/headers da revisão fixada. Não presumir nomes de arquivos ou opções de build.

## 3. Arquitetura e fluxo de áudio

Fluxo esperado:

```text
Audio Device Callback
    -> FIFO existente
    -> VoiceEngine worker
       -> Input Equalizer (se habilitado)
       -> RNNoise (se habilitado)
          -> conversão para 48 kHz mono
          -> frames exatos de 480 amostras
          -> processamento RNNoise com estado persistente
          -> conversão de volta à taxa nativa
       -> PitchDetector / YIN
       -> NoteTracker / Auto-Tune / MIDI existentes
```

Regras obrigatórias:
- Não executar RNNoise, resampling, locks bloqueantes, alocação dinâmica ou I/O na callback de áudio.
- O estado recorrente do RNNoise deve persistir entre frames e ser reinicializado somente em eventos controlados.
- Acumular amostras entre chamadas process() até completar frames de 480 amostras. Não descartar nem duplicar amostras em blocos parciais.
- Tratar blocos variáveis, inclusive menores que 480 amostras e chamadas com vários frames.
- Preparar buffers e memória antecipadamente em prepare(). O caminho de processamento não deve alocar memória.
- Garantir que o processamento nunca escreva fora dos limites dos buffers.
- Se houver falha de inicialização ou configuração não suportada, degradar para bypass seguro e observável.

## 4. Taxa de amostragem e latência

RNNoise opera no formato esperado pela API oficial: 48 kHz, mono, frames de 480 amostras. O NOVOX não pode presumir que o dispositivo esteja em 48 kHz.

Implementar conversão de taxa de amostragem contínua e stateful, com buffers e fase persistentes. Não usar conversão ingênua independente por bloco, pois isso pode causar descontinuidades, drift, perda/duplicação de amostras e instabilidade no pitch.

Cobrir no mínimo 44.1 kHz, 48 kHz (caminho direto), 88.2 kHz e 96 kHz.

A conversão de retorno deve preservar continuidade temporal e número de amostras. Documentar a latência adicional introduzida pelo framing e pelos filtros de resampling. Medir e reportar o custo real; evitar buffers excessivos e espera arbitrária.

## 5. API e compatibilidade NOVOX

- Preservar os métodos públicos usados por VoiceEngine e MainComponent, ou atualizar todos os chamadores no mesmo conjunto de alterações.
- Preservar o toggle de redução de ruído e a persistência de configurações.
- O slider antigo de intensidade e a calibração de noise floor foram removidos da interface; o campo legado de intensidade em presets é ignorado para compatibilidade de leitura.
- Avaliar o controle atual de intensidade: manter semântica real e documentada, ou removê-lo coerentemente da UI, configurações e presets. Não manter slider sem efeito.
- A calibração de noise floor é específica do gate. Remover ou redefinir deliberadamente calibração, estados, textos, settings e referências. Não deixar API/UI morta nem fingir que RNNoise usa essa calibração.
- Atualizar o rótulo da UI para RNNoise ou Noise reduction (RNNoise); não continuar chamando o recurso de gate.
- Preservar leitura de configurações antigas sem crash. Valores legados de calibração podem ser ignorados com migração segura.
- Garantir que presets existentes continuem carregando; definir compatibilidade para o campo antigo de intensidade.

## 6. Qualidade de áudio e segurança numérica

- Sanitizar entradas/saídas não finitas quando necessário, sem mascarar falhas silenciosamente.
- Evitar clipping ou ganho inesperado na saída do denoiser.
- Não zerar blocos incompletos nem produzir silêncio enquanto espera frame: definir e testar estratégia correta de buffering/saída compatível com o pipeline em tempo real.
- Evitar alteração de pitch, instabilidade de oitava, perda de consoantes/ataques e modulação audível.
- Não confundir supressão de ruído neural com gate tradicional.
- Bypass desativado deve ser bit-transparent quando possível; preferir bypass sem atraso adicional.

## 7. Testes automatizados obrigatórios

Substituir testes antigos específicos do gate/calibração por testes do wrapper RNNoise. Não testar apenas compilação.

1. Bypass: desativado preserva o sinal exatamente e não altera estado de áudio desnecessariamente.
2. Inicialização: prepare/reprepare em cada taxa suportada, inclusive mudanças de taxa e tamanho máximo de bloco.
3. Frames parciais: chamadas irregulares não perdem, duplicam ou corrompem amostras.
4. Frames múltiplos: chamadas maiores que 480 amostras processam todos os frames necessários.
5. Continuidade: blocos consecutivos não apresentam descontinuidades artificiais na junção.
6. Estabilidade numérica: silêncio, ruído, senoides e voz sintética não geram NaN/Inf nem escrita fora do buffer.
7. Reset/enable: alternar habilitado/desabilitado e reconfigurar não deixa estado inválido.
8. Resampling: 44.1/48/88.2/96 kHz mantém duração, continuidade e contagem de amostras dentro da tolerância definida.
9. Redução de ruído: sinal contaminado com ruído apresenta redução mensurável do componente de ruído, sem afirmar que teste sintético representa toda voz real.
10. Worker integration: processamento permanece fora da callback e o pipeline YIN/MIDI continua conectado.

Usar critérios determinísticos e tolerâncias documentadas; evitar asserts frágeis dependentes de pequenas diferenças entre compiladores.

## 8. Build e CI Windows

- Integrar as fontes C oficiais pelo CMake de forma reproduzível no Windows/MSVC e compatível com o setup atual.
- Garantir compilação C e C++ com flags adequadas por linguagem; não aplicar flags C++ incompatíveis às fontes C.
- Não depender de Meson/Autotools no runner, a menos que isso seja explicitamente configurado e validado na CI.
- Atualizar targets de teste e warnings do CMake.
- Executar workflow Windows Release e todos os testes CTest.
- Inspecionar logs completos de build/testes; não declarar sucesso com base em build parcial.
- Publicar artefato executável somente após CI verde.

## 9. Restrições de escopo

Não reescrever YIN; não alterar Auto-Tune, NoteTracker, MIDI cleanup, pitch bend ou roteamento MIDI; não executar DSP pesado na callback; não redesenhar a interface; não alterar outras branches nem fazer merge em main.

Mudanças na UI são permitidas apenas para refletir corretamente RNNoise e retirar controles de calibração incompatíveis.

## 10. Entrega esperada

- [ ] RNNoise oficial fixado por versão/commit e licença incluída.
- [ ] CMake compila a biblioteca no Windows/MSVC.
- [ ] Wrapper sem alocações no caminho de áudio, com buffering correto.
- [ ] Resampling stateful para taxas suportadas.
- [ ] Integração no worker entre EQ e PitchDetector.
- [x] Remoção dos controles de calibração e intensidade sem efeito; leitura de presets antigos preservada pelo campo legado.
- [ ] Resampling contínuo para taxas diferentes de 48 kHz.
- [ ] Testes automatizados cobrindo a seção 7.
- [ ] CI Windows Release verde e CTest completo verde.
- [ ] Artefato executável gerado e pronto para validação auditiva.
- [ ] Resumo final com arquivos alterados, decisões técnicas, latência medida, testes e commit SHA.

## 11. Critério final de aceite

A tarefa só pode ser marcada como concluída quando a CI Windows Release compilar o aplicativo e todos os testes passarem; RNNoise estiver realmente executando no worker; taxas diferentes de 48 kHz funcionarem por resampling contínuo; bypass e alternância forem seguros; o fluxo YIN/MIDI não sofrer regressão deliberada; e o executável estiver disponível para teste com voz real.

**Não declarar integrado, validado ou pronto antes desses critérios.**


## 12. Referências técnicas verificadas para a implementação

A investigação da API upstream foi feita na revisão imutável abaixo (não usar `main` como dependência de build):

- Repositório: https://github.com/xiph/rnnoise
- Commit fixado para a integração: `70f1d256acd4b34a572f999a05c87bf00b67730d`
- Header público: `include/rnnoise.h`
- API de processamento: `rnnoise_create`, `rnnoise_process_frame`, `rnnoise_destroy`.
- Tamanho de frame reportado pela API: 480 amostras.
- A demonstração oficial lê PCM16 e converte diretamente cada amostra `short` para `float` antes de chamar `rnnoise_process_frame`. Portanto, o wrapper NOVOX deve converter float JUCE normalizado para a escala PCM esperada (multiplicação por 32768) e normalizar a saída novamente, com proteção contra valores não finitos e clipping.

Modelo correspondente ao arquivo `model_version` dessa revisão:

- SHA-256: `0a8755f8e2d834eff6a54714ecc7d75f9932e845df35f8b59bc52a7cfe6e8b37`
- Arquivo oficial: `https://media.xiph.org/rnnoise/models/rnnoise_data-0a8755f8e2d834eff6a54714ecc7d75f9932e845df35f8b59bc52a7cfe6e8b37.tar.gz`
- O script oficial `download_model.sh` valida o SHA-256 do arquivo compactado antes de extraí-lo.
- A API permite carregar pesos com `rnnoise_model_from_filename` ou `rnnoise_model_from_buffer`; ao usar modelo customizado, liberar o estado de denoise antes de liberar o modelo. O arquivo/bytes do modelo devem estar disponíveis localmente e nunca ser baixados pelo aplicativo em tempo de execução.

Observação importante: o modelo e o código são dependências distintas. O CMake/CI deve validar o download e checksum do pacote de modelo, extrair/gerar os arquivos necessários e empacotar o modelo junto ao executável (ou incorporá-lo como recurso). Não habilitar silenciosamente `rnnoise_create(NULL)` caso os pesos default gerados não estejam presentes. O teste de inicialização precisa falhar de forma explícita e o NOVOX deve permanecer em bypass seguro se o modelo não carregar.
