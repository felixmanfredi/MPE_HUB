export interface MCUData {
  tensione: number;
  corrente: number;
  potenza: number;
  state: string;
  isPowered: boolean;
  isConnected: boolean;
  temperatura: number;
  water: boolean;
  flashlights: boolean;
  V5v: number;
  V3v3: number;
  isFlashing: boolean;
  isUSB: boolean;
  expTime: number;
  //flashPulseTime: number;
}