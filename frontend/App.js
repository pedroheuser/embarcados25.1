import React, { useEffect, useState } from 'react';
import {
  SafeAreaView,
  StatusBar,
  StyleSheet,
  Text,
  View,
  TouchableOpacity,
  ScrollView,
  Alert,
  Dimensions
} from 'react-native';
import axios from 'axios';
import { LineChart } from 'react-native-chart-kit';
import Slider from '@react-native-community/slider';
import { LinearGradient } from 'expo-linear-gradient';

// Defina o IP/URL do seu VPS da UFSC aqui
const API_URL = 'http://embarcados.pedro.heuser.vms.ufsc.br/api';
const SCREEN_WIDTH = Dimensions.get('window').width;

const App = () => {
  const [luminosidade, setLuminosidade] = useState(0);
  const [chartData, setChartData] = useState([0, 0, 0, 0, 0, 0]);
  const [statusLuz, setStatusLuz] = useState('...');
  const [modoAtual, setModoAtual] = useState('auto'); // auto, manual, pisca, fade
  const [statusApi, setStatusApi] = useState('Conectando...');
  const [speed, setSpeed] = useState(1000); // Velocidade em ms (para pisca/fade)

  // Cores predefinidas
  const colors = [
    { name: 'Vermelho', r: 255, g: 0, b: 0, hex: '#FF453A' },
    { name: 'Verde', r: 0, g: 255, b: 0, hex: '#32D74B' },
    { name: 'Azul', r: 0, g: 0, b: 255, hex: '#0A84FF' },
    { name: 'Roxo', r: 128, g: 0, b: 128, hex: '#BF5AF2' },
    { name: 'Laranja', r: 255, g: 165, b: 0, hex: '#FF9F0A' },
    { name: 'Branco', r: 255, g: 255, b: 255, hex: '#FFFFFF' },
    { name: 'Desligar', r: 0, g: 0, b: 0, hex: '#1C1C1E' },
  ];

  const modes = [
    { id: 'auto', label: 'Automático', icon: '🤖' },
    { id: 'manual', label: 'Manual', icon: '🎨' },
    { id: 'pisca', label: 'Pisca', icon: '⚡' },
    { id: 'fade', label: 'Fade', icon: '🌊' },
  ];

  // Função para buscar dados da API (Luminosidade)
  const fetchLuminosidade = async () => {
    try {
      const response = await axios.get(`${API_URL}/luminosidade/`);
      const valor = response.data.valor;

      setLuminosidade(valor);
      setStatusLuz(response.data.modo);
      setStatusApi('Online');

      // Atualiza o gráfico (mantém últimos 6 pontos)
      setChartData(prev => {
        const newData = [...prev, valor];
        if (newData.length > 6) newData.shift();
        return newData;
      });

    } catch (error) {
      console.error('Erro ao buscar dados:', error);
      setStatusApi('Offline');
    }
  };

  // Função para enviar comando de controle
  const sendCommand = async (modo, cor = null, velocidade = null) => {
    try {
      const payload = { modo };
      if (cor) {
        payload.cor = { r: cor.r, g: cor.g, b: cor.b };
      }
      // Envia velocidade se for modo pisca ou fade
      if (velocidade && (modo === 'pisca' || modo === 'fade')) {
        payload.velocidade = velocidade;
      }

      // Otimistic update
      setModoAtual(modo);

      await axios.post(`${API_URL}/controle/`, payload);
      console.log('Comando enviado:', payload);
    } catch (error) {
      console.error('Erro ao enviar comando:', error);
      // Não mostramos alert para não interromper a experiência se for frequente
    }
  };

  // Buscar estado inicial do controle
  const fetchControlState = async () => {
    try {
      const response = await axios.get(`${API_URL}/controle/`);
      if (response.data.modo) {
        setModoAtual(response.data.modo);
      }
    } catch (error) {
      console.log('Erro ao buscar estado inicial:', error);
    }
  }

  useEffect(() => {
    fetchLuminosidade();
    fetchControlState();
    const interval = setInterval(fetchLuminosidade, 3000); // Poll a cada 3s
    return () => clearInterval(interval);
  }, []);

  const handleModeSelect = (modeId) => {
    setModoAtual(modeId);
    // Se for Auto, envia direto. Outros modos esperam interação ou enviam padrão.
    if (modeId === 'auto') {
      sendCommand(modeId);
    } else if (modeId === 'fade') {
      sendCommand(modeId, null, speed);
    } else {
      // Manual e Pisca enviam apenas o modo por enquanto
      sendCommand(modeId);
    }
  };

  const handleColorSelect = (color) => {
    sendCommand(modoAtual, color, speed);
  };

  const handleSpeedChange = (val) => {
    setSpeed(val);
  };

  const handleSpeedComplete = (val) => {
    // Envia comando com nova velocidade quando soltar o slider
    sendCommand(modoAtual, null, val);
  };

  return (
    <LinearGradient
      colors={['#1c1c1e', '#2c3e50', '#000000']}
      style={styles.container}
    >
      <SafeAreaView style={{ flex: 1 }}>
        <StatusBar barStyle="light-content" backgroundColor="transparent" translucent />

        <ScrollView contentContainerStyle={styles.scrollContent}>
          {/* Header */}
          <View style={styles.header}>
            <View>
              <Text style={styles.headerTitle}>IoT Control</Text>
              <Text style={styles.headerSubtitle}>Smart Lighting System</Text>
            </View>
            <View style={[styles.statusBadge, statusApi === 'Online' ? styles.badgeOnline : styles.badgeOffline]}>
              <Text style={styles.statusBadgeText}>{statusApi}</Text>
            </View>
          </View>

          {/* Monitoramento Gráfico */}
          <View style={styles.card}>
            <View style={styles.cardHeader}>
              <Text style={styles.cardTitle}>Luminosidade em Tempo Real</Text>
              <Text style={styles.cardValue}>{luminosidade} <Text style={styles.unitText}>LDR</Text></Text>
            </View>

            <View style={styles.chartContainer}>
              <LineChart
                data={{
                  labels: [], // Sem labels para limpar o visual
                  datasets: [{ data: chartData }]
                }}
                width={SCREEN_WIDTH - 90} // Largura ajustada para caber no card
                height={180}
                yAxisInterval={1}
                withDots={true}
                withInnerLines={false}
                withOuterLines={false}
                withVerticalLines={false}
                withHorizontalLines={false}
                chartConfig={{
                  backgroundColor: "transparent",
                  backgroundGradientFrom: "transparent",
                  backgroundGradientTo: "transparent",
                  decimalPlaces: 0,
                  color: (opacity = 1) => `rgba(10, 132, 255, ${opacity})`,
                  labelColor: (opacity = 1) => `rgba(255, 255, 255, ${opacity})`,
                  style: { borderRadius: 16 },
                  propsForDots: {
                    r: "4",
                    strokeWidth: "2",
                    stroke: "#30D158"
                  }
                }}
                bezier
                style={styles.chart}
              />
            </View>
            <Text style={styles.statusText}>Status Atual: {statusLuz}</Text>
          </View>

          {/* Seletor de Modos */}
          <Text style={styles.sectionTitle}>Modo de Operação</Text>
          <View style={styles.modeContainer}>
            {modes.map((m) => (
              <TouchableOpacity
                key={m.id}
                style={[
                  styles.modeButton,
                  modoAtual === m.id && styles.modeButtonActive
                ]}
                onPress={() => handleModeSelect(m.id)}
              >
                <Text style={styles.modeIcon}>{m.icon}</Text>
                <Text style={[
                  styles.modeText,
                  modoAtual === m.id && styles.modeTextActive
                ]}>{m.label}</Text>
              </TouchableOpacity>
            ))}
          </View>

          {/* Controle de Velocidade (Pisca/Fade) */}
          {(modoAtual === 'pisca' || modoAtual === 'fade') && (
            <View style={styles.card}>
              <Text style={styles.cardTitle}>Velocidade do Efeito</Text>
              <View style={styles.sliderContainer}>
                <Text style={styles.sliderLabel}>Rápido</Text>
                <Slider
                  style={{ flex: 1, height: 40 }}
                  minimumValue={100}
                  maximumValue={2000}
                  step={100}
                  value={speed}
                  onValueChange={handleSpeedChange}
                  onSlidingComplete={handleSpeedComplete}
                  minimumTrackTintColor="#0A84FF"
                  maximumTrackTintColor="#545458"
                  thumbTintColor="#FFFFFF"
                />
                <Text style={styles.sliderLabel}>Lento</Text>
              </View>
              <Text style={styles.speedValue}>{speed} ms</Text>
            </View>
          )}

          {/* Seletor de Cores (Manual/Pisca) */}
          {(modoAtual === 'manual' || modoAtual === 'pisca') && (
            <View style={styles.colorSection}>
              <Text style={styles.sectionTitle}>Paleta de Cores</Text>
              <View style={styles.colorGrid}>
                {colors.map((c, index) => (
                  <TouchableOpacity
                    key={index}
                    style={[styles.colorButton, { backgroundColor: c.hex }]}
                    onPress={() => handleColorSelect(c)}
                  >
                    {/* Indicador de seleção se necessário */}
                  </TouchableOpacity>
                ))}
              </View>
            </View>
          )}

        </ScrollView>
      </SafeAreaView>
    </LinearGradient>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  scrollContent: {
    padding: 20,
    paddingBottom: 50,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 25,
    marginTop: 10,
  },
  headerTitle: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#FFFFFF',
  },
  headerSubtitle: {
    fontSize: 14,
    color: 'rgba(255,255,255,0.6)',
  },
  statusBadge: {
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 20,
    borderWidth: 1,
    borderColor: 'rgba(255,255,255,0.1)',
  },
  badgeOnline: {
    backgroundColor: 'rgba(50, 215, 75, 0.2)',
    borderColor: '#32D74B',
  },
  badgeOffline: {
    backgroundColor: 'rgba(255, 69, 58, 0.2)',
    borderColor: '#FF453A',
  },
  statusBadgeText: {
    color: '#FFFFFF',
    fontSize: 12,
    fontWeight: 'bold',
  },
  card: {
    backgroundColor: 'rgba(28, 28, 30, 0.6)',
    borderRadius: 20,
    padding: 20,
    marginBottom: 25,
    borderWidth: 1,
    borderColor: 'rgba(255,255,255,0.1)',
  },
  cardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  cardTitle: {
    fontSize: 16,
    color: 'rgba(255,255,255,0.8)',
    fontWeight: '600',
  },
  cardValue: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#0A84FF',
  },
  unitText: {
    fontSize: 14,
    color: 'rgba(255,255,255,0.4)',
    fontWeight: 'normal',
  },
  chartContainer: {
    alignItems: 'center',
    justifyContent: 'center',
    marginVertical: 10,
  },
  chart: {
    borderRadius: 16,
    paddingRight: 0,
    paddingLeft: 0,
  },
  statusText: {
    textAlign: 'center',
    color: 'rgba(255,255,255,0.5)',
    fontSize: 14,
    marginTop: 5,
  },
  sectionTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#FFFFFF',
    marginBottom: 15,
    marginLeft: 5,
  },
  modeContainer: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
    marginBottom: 25,
  },
  modeButton: {
    width: '48%',
    backgroundColor: 'rgba(44, 44, 46, 0.6)',
    paddingVertical: 20,
    borderRadius: 16,
    alignItems: 'center',
    marginBottom: 12,
    borderWidth: 1,
    borderColor: 'rgba(255,255,255,0.05)',
  },
  modeButtonActive: {
    backgroundColor: '#0A84FF',
    borderColor: '#0A84FF',
    shadowColor: "#0A84FF",
    shadowOffset: { width: 0, height: 4 },
    shadowOpacity: 0.3,
    shadowRadius: 8,
  },
  modeIcon: {
    fontSize: 24,
    marginBottom: 8,
  },
  modeText: {
    fontSize: 16,
    color: 'rgba(255,255,255,0.6)',
    fontWeight: '600',
  },
  modeTextActive: {
    color: '#FFFFFF',
    fontWeight: 'bold',
  },
  sliderContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 10,
  },
  sliderLabel: {
    color: 'rgba(255,255,255,0.5)',
    fontSize: 12,
    width: 45,
    textAlign: 'center',
  },
  speedValue: {
    textAlign: 'center',
    color: '#0A84FF',
    fontWeight: 'bold',
    marginTop: 5,
  },
  colorSection: {
    marginBottom: 20,
  },
  colorGrid: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'space-between',
  },
  colorButton: {
    width: (SCREEN_WIDTH - 40) / 4 - 10,
    height: (SCREEN_WIDTH - 40) / 4 - 10,
    borderRadius: 30,
    marginBottom: 15,
    borderWidth: 2,
    borderColor: 'rgba(255,255,255,0.2)',
  },
});

export default App;

