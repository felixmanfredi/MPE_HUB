// src/app/services/tick.service.ts
import { Injectable } from '@angular/core';
import { interval, Subject } from 'rxjs';
import { environment } from '../environments/environment';

@Injectable({
  providedIn: 'root'
})
export class TickService {
  private tickSubject = new Subject<number>();
  private tickCounter = 0;

  constructor() {
    interval(environment.timeUpdate).subscribe(() => {
      this.tickSubject.next(this.tickCounter++);
    });
  }

  get tick$() {
    return this.tickSubject.asObservable();
  }
}
