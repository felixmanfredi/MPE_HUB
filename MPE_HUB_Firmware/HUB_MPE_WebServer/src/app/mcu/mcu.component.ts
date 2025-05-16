import { Component, Input, OnInit } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { MCUData } from './mcu.model';
import { interval } from 'rxjs';
import { CommonModule } from '@angular/common';
import { HttpClient, HttpClientModule } from '@angular/common/http';
//import { MCUService } from './mcu.service';
import { environment } from '../environments/environment';
import { TickService } from '../services/tick.service'; // <-- IMPORTA IL SERVIZIO
import { Subscription } from 'rxjs';

@Component({
  selector: 'app-mcu',
  standalone: true,
  //imports: [CommonModule, HttpClientModule, MdbFormsModule, MatCardModule, MatIconModule, MatButtonModule],
  imports: [FormsModule, CommonModule, HttpClientModule],
  templateUrl: './mcu.component.html',
  styleUrls: ['./mcu.component.css']
})
export class MCUComponent implements OnInit {
  mcuData: MCUData | null = null; // Utilizza il modello
  loading = true;
  error: string | null = null;
  isUpdating = false;
  //flashPulseTime = null; // Tempo impostabile da HTML
  isFlashing = false;
  //newflashPulseTime = 0;
  flashPulseTime = 0;
  private componentId = environment.mcuID; // ID UNIVOCO per questo componente
  private tickSub!: Subscription;


  private apiUrl = environment.apiUrl;

  constructor(private http: HttpClient, private tickService: TickService) { }

  ngOnInit(): void {
    this.tickSub = this.tickService.tick$.subscribe(tick => {
      // Esegui la richiesta solo se il tick è allineato con il tuo ID
      if (tick % 4 === this.componentId) {
        this.loadMCUData();
      }
    });
  }

  ngOnDestroy(): void {
    if (this.tickSub) {
      this.tickSub.unsubscribe();
    }
  }

  loadMCUData(): void {
  
    this.loading = true;
    this.error = null;
    this.http.get<MCUData>(`${this.apiUrl}/api/mcu`).subscribe({
      next: (data) => {
        this.mcuData = data;
        this.loading = false;
      },
      error: (err) => {
        this.error = 'Errore nel caricamento dei dati.';
        console.error(err);
        this.loading = false;
      }
    });
    //console.log(this.mcuData);
  }



  triggerFlash() {
    if (this.isUpdating) return;
  
    this.isUpdating = true;
    this.isFlashing = true;
  
    // Invia POST al server
    //this.http.post('/api/mcu/flashlights', { flashPulseTime: this.mcuData?.flashPulseTime })
    
    //this.http.post('/api/mcu/flashlights', { flashPulseTime: this.flashPulseTime })
    this.http.post('/api/mcu/flashlights', { isFlashing: this.isFlashing })
      .subscribe({
        next: () => {
          // Attendi la durata dell'impulso prima di spegnere animazione
          setTimeout(() => {
            this.isFlashing = false;
            this.isUpdating = false;
          }, this.flashPulseTime);
          //}, this.mcuData?.flashPulseTime);
        },
        error: (err) => {
          console.error('Errore POST:', err);
          this.isFlashing = false;
          this.isUpdating = false;
        }
      });
  }

  togglePower(): void {
    if (this.isUpdating) return;
    this.isUpdating = true;
    if (this.mcuData) {
      //this.http.post<MCUData>(`${this.apiUrl}/mcu`, { state: !this.mcuData.isPowered }).subscribe({
      this.http.post<MCUData>(`${this.apiUrl}/api/mcu/flashlights`, { flashlights: !this.mcuData.flashlights }).subscribe({
        next: (data) => {
          this.mcuData = data;
          this.isUpdating = false;
        },
        error: (err) => {
          console.error('Errore nell\'aggiornamento dello stato di alimentazione.', err);
          this.isUpdating = false;
        }
      });
    }
  }

    getStateColor(): string {
      if (this.mcuData) {
      switch(this.mcuData.state) {
        case 'normal':
          return 'green';
        case 'idle':
          return 'blue';
        case 'watning':
          return 'orange';
        case 'error':
          return 'red';
        default:
          return 'gray';
      }
    }
    return '';
  }

  getStatusColor(isConnected: boolean): string {
    if (isConnected) {
      return '#4CAF50'; // Verde
    } else {
      return '#F44336'; // Rosso
    }
  }

  // getStateClass(): string {
  //   if (this.mcuData) {
  //     switch (this.mcuData.state) {
  //       case 'warning':
  //         return 'warning';
  //       case 'error':
  //         return 'error';
  //       case 'idle':
  //         return 'idle';
  //       case 'normal':
  //       default:
  //         return 'normal';
  //     }
  //   }
  //   return '';
  // }


  
}
