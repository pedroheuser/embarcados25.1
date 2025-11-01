import React, { useEffect, useState } from 'react';
import { Button, SafeAreaView, StatusBar, StyleSheet, Text, View } from 'react-native';
import axios from 'axios';

// Defina o IP do seu VPS da UFSC aqui
// IMPORTANTE: Em desenvolvimento, se o backend rodar no seu PC, use o IP da sua máquina na rede (ex: 192.168.1.10), não 'localhost'.
const API_URL = 'http://<IP_DO_VPS_UFSC>:8000/api';

const App = () => {
  // 'state' é necessário para apps dinâmicos [cite: 10, 82]
  const [luminosidade, setLuminosidade] = useState(0);
  const [modo, setModo] = useState('...');
  const [statusApi, setStatusApi] = useState('Offline');

  // Função para buscar dados da API
  const fetchLuminosidade = async () => {
    try {
      const response = await axios.get(`${API_URL}/luminosidade/`);
      setLuminosidade(response.data.valor);
      setModo(response.data.modo);
      setStatusApi('Online');
    } catch (error) {
      console.error('Erro ao buscar dados:', error);
      setStatusApi('Erro');
    }
  };

  // Função para enviar controle manual
  const sendControleManual = async (cor) => {
    try {
      const response = await axios.post(`${API_URL}/controle/`, {
        modo: 'manual',
        cor: cor,
      });
      console.log('Controle manual enviado:', response.data);
    } catch (error) {
      console.error('Erro ao enviar controle:', error);
    }
  };

  // Efeito para carregar os dados quando o app abre
  useEffect(() => {
    fetchLuminosidade();
    // Opcional: atualizar a cada X segundos
    const interval = setInterval(fetchLuminosidade, 5000);
    return () => clearInterval(interval); // Limpa o intervalo
  }, []);

  // Usamos 'View' como container e 'Text' para exibir texto [cite: 15, 29]
  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="dark-content" />
      <Text style={styles.title}>Controle de Iluminação RGB</Text>

      <View style={styles.statusBox}>
        <Text style={styles.statusText}>Status da API: {statusApi}</Text>
        <Text style={styles.statusText}>Luminosidade: {luminosidade}</Text>
        <Text style={styles.statusText}>Modo Atual: {modo}</Text>
      </View>

      <View style={styles.controls}>
        <Text style={styles.subtitle}>Controle Manual</Text>
        <Button
          title="Luz Vermelha"
          onPress={() => sendControleManual({ r: 255, g: 0, b: 0 })}
          color="#e74c3c"
        />
        <Button
          title="Luz Azul"
          onPress={() => sendControleManual({ r: 0, g: 0, b: 255 })}
          color="#3498db"
        />
        <Button
          title="Desligar"
          onPress={() => sendControleManual({ r: 0, g: 0, b: 0 })}
          color="#7f8c8d"
        />
      </View>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
    alignItems: 'center',
    padding: 20,
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    marginBottom: 20,
  },
  statusBox: {
    backgroundColor: '#ffffff',
    padding: 20,
    borderRadius: 10,
    width: '100%',
    marginBottom: 20,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  statusText: {
    fontSize: 18,
    marginBottom: 8,
  },
  controls: {
    width: '100%',
  },
  subtitle: {
    fontSize: 20,
    fontWeight: '600',
    marginBottom: 10,
    textAlign: 'center',
  },
});

export default App;

