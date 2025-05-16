import { Component } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { JetsonComponent } from './jetson/jetson.component';
import { SonyComponent } from './sony/sony.component';
import { MCUComponent } from "./mcu/mcu.component";
import { VideoencComponent } from './videoenc/videoenc.component';

@Component({
  selector: 'app-root',
  //imports: [RouterOutlet, JetsonComponent],
  imports: [JetsonComponent, SonyComponent, VideoencComponent, MCUComponent],
  //imports: [CameraComponent],//, VideoencComponent],
  templateUrl: './app.component.html',
  styleUrl: './app.component.css'
})
export class AppComponent {
  title = 'my-app';
}
